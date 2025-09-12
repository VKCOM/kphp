// Compiler for PHP (aka KPHP)
// Copyright (c) 2025 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <expected>
#include <iterator>
#include <memory>
#include <optional>
#include <span>
#include <string_view>
#include <utility>

#include "runtime-common/core/allocator/script-malloc-interface.h"
#include "runtime-light/coroutine/event.h"
#include "runtime-light/coroutine/task.h"
#include "runtime-light/stdlib/diagnostics/logs.h"
#include "runtime-light/streams/stream.h"
#include "runtime-light/tl/tl-core.h"
#include "runtime-light/tl/tl-functions.h"
#include "runtime-light/tl/tl-types.h"

namespace kphp::component::InterComponentSession {

// Inter component communicator for client-server interactions over a stream in 'keep-alive' manner
class Client {
  using query_id_type = uint64_t;
  using query2notifier_type = kphp::stl::unordered_map<query_id_type, kphp::coro::event, kphp::memory::script_allocator>;

  struct refcountable_transport_type : public refcountable_php_classes<refcountable_transport_type> {
    kphp::component::stream stream;
  };
  using shared_transport_type = class_instance<refcountable_transport_type>;

  shared_transport_type transport;
  query_id_type query_count{};

  // WRITER
  struct writer {
    bool is_occupied{false};
    query_id_type occupied_by{0};
    query2notifier_type query2notifier{};
    kphp::stl::queue<query_id_type, kphp::memory::script_allocator> queue;

    inline auto write(shared_transport_type t, query_id_type qid,
                      std::span<const std::byte> payload) noexcept -> kphp::coro::task<std::expected<void, int32_t>> {
      // Wait until transport will be available
      if (is_occupied) [[unlikely]] {
        query2notifier.emplace(qid, kphp::coro::event{});
        queue.push(qid);
        co_await query2notifier[qid];
      }

      // Ensure that transport is available
      kphp::log::assertion(is_occupied == false);
      // Capture the transport
      is_occupied = true, occupied_by = qid;

      // Write request size
      tl::InterComponentSessionRequestHeader req_header{.size = {payload.size_bytes()}};
      tl::storer tls{req_header.footprint()};
      req_header.store(tls);
      if (auto res{co_await t.get()->stream.write(tls.view())}; !res) [[unlikely]] {
        co_return std::unexpected{res.error()};
      }

      // Ensure the transport we still held by us
      kphp::log::assertion(is_occupied && occupied_by == qid);

      // Write payload
      if (auto res{co_await t.get()->stream.write(payload)}; !res) [[unlikely]] {
        co_return std::unexpected{res.error()};
      }

      // Double check
      kphp::log::assertion(is_occupied && occupied_by == qid);

      // Release transport
      is_occupied = false;

      // Notify next one that transport is available
      if (!queue.empty()) {
        auto q{queue.front()};
        queue.pop();
        query2notifier[q].set();
      }
      co_return std::expected<void, int32_t>{};
    }
  } writer;

  // READER
  struct reader {
    struct refcountable_ctx_type : public refcountable_php_classes<refcountable_ctx_type> {
      query2notifier_type query2notifier{};
      kphp::stl::unordered_map<query_id_type, kphp::stl::vector<std::byte, kphp::memory::script_allocator>&, kphp::memory::script_allocator>
          query2resp_buffer{};
      kphp::stl::unordered_map<query_id_type, std::span<std::byte>, kphp::memory::script_allocator> query2resp{};
      bool is_running{true};
      std::optional<int32_t> error{};
    };
    using shared_ctx_type = class_instance<refcountable_ctx_type>;

    // The following reader state is intended to be initialized once for a new client.
    // It is assumed that "copying" (ref count increasing) will be the common case, rather than moving.
    shared_ctx_type ctx;
    kphp::coro::shared_task<void> runner;

    reader(shared_ctx_type _ctx, shared_transport_type _t) noexcept
        : ctx(_ctx),
          runner(reader::run(_ctx, _t)){};

    static inline auto run(shared_ctx_type ctx, shared_transport_type t) noexcept -> kphp::coro::shared_task<void> {
      // Allocate buffer for header
      tl::InterComponentSessionResponseHeader resp_header{};
      std::array<std::byte, resp_header.footprint()> resp_header_buf{};

      while (ctx.get()->is_running) {
        // Read response header
        if (auto res{co_await t.get()->stream.read(resp_header_buf)}; !res) [[unlikely]] {
          ctx.get()->error = res.error();
          break;
        }

        // Parse response id and size
        tl::fetcher tlf{resp_header_buf};
        if (!resp_header.fetch(tlf)) [[unlikely]] {
          ctx.get()->error = k2::errno_einval;
          break;
        }
        const auto qid{resp_header.id.value};
        const auto size{resp_header.size.value};

        // Ensure that buffer for response is presented
        kphp::log::assertion(ctx.get()->query2resp_buffer.contains(qid));

        // Reserve enough space for appending response payload
        auto& resp_payload_buf{ctx.get()->query2resp_buffer.at(qid)};
        const auto prev_buf_size{resp_payload_buf.size()};
        resp_payload_buf.resize(prev_buf_size + size);

        // Ensure that buffer successfully resized
        if (resp_payload_buf.size() < prev_buf_size + size) [[unlikely]] {
          ctx.get()->error = k2::errno_enomem;
          break;
        }

        // Make slice for response
        auto resp{std::span<std::byte>{std::next(resp_payload_buf.begin(), prev_buf_size), std::next(resp_payload_buf.begin(), prev_buf_size + size)}};
        ctx.get()->query2resp[qid] = resp;

        // Read payload
        if (auto res{co_await t.get()->stream.read(resp)}; !res) [[unlikely]] {
          ctx.get()->error = res.error();
          break;
        }

        // Ensure that notifier is presented and notify
        kphp::log::assertion(ctx.get()->query2notifier.contains(qid));
        ctx.get()->query2notifier[qid].set();
      }

      // Error occurred, notify all waiting queries
      for (auto& [_, notifier] : ctx.get()->query2notifier) {
        if (!notifier.is_set()) [[unlikely]] {
          notifier.set();
        }
      }
    }

    inline auto register_query(query_id_type qid, kphp::stl::vector<std::byte, kphp::memory::script_allocator>& buffer) noexcept -> void {
      // We wouldn't read a response twice
      kphp::log::assertion(ctx.get()->query2notifier.contains(qid) == false);

      // Register storage for a response
      ctx.get()->query2resp_buffer.insert({qid, buffer});

      // Register notifier
      ctx.get()->query2notifier.emplace(qid, kphp::coro::event{});
    }
  } reader;

  explicit Client(kphp::component::stream&& s)
      : transport(make_instance<refcountable_transport_type>(refcountable_transport_type{.stream = std::move(s)})),
        reader({make_instance<reader::refcountable_ctx_type>(), transport}) {
    // Run reader as "service"
    kphp::coro::io_scheduler::get().start(reader.runner);
  }

public:
  ~Client() {
    // If client has been moved, skip disabling the reader.
    // Otherwise, shut down the reader.
    if (query_count != std::numeric_limits<query_id_type>::max()) {
      reader.ctx.get()->is_running = false;
    }
  }

  // Designed that transport and reader will be allocated once as refcountable class instance.
  // Moving looks like copying but is simply reference count increasing for 'transport' and 'reader' fields.
  // Such approach motivated by the fact that the "reader-service" cannot be easily moved due to depends on transport and cannot be trivial stopped.
  Client(Client&& other) noexcept
      : transport(other.transport), // Intentionally call of copy constructor for shared transport
        query_count(std::exchange(other.query_count, std::numeric_limits<query_id_type>::max())),
        writer(std::move(other.writer)),
        reader(other.reader) /* Intentionally call of copy constructor for shared reader  */ {}

  auto operator=(Client&& other) noexcept -> Client& {
    if (this != std::addressof(other)) {
      transport = other.transport; // Intentionally call of copy assignment for shared transport
      query_count = std::exchange(other.query_count, std::numeric_limits<query_id_type>::max());
      writer = std::move(other.writer);
      reader = other.reader; // Intentionally call of copy assignment for shared reader
    }
    return *this;
  }

  Client(const Client&) = delete;
  auto operator=(const Client&) -> Client& = delete;

  static inline auto create(std::string_view component_name) noexcept -> std::expected<Client, int32_t>;

  template<std::invocable<std::span<std::byte>> F>
  requires std::convertible_to<kphp::coro::async_function_return_type_t<F, std::span<std::byte>>, bool>
  inline auto query(std::span<const std::byte> request, kphp::stl::vector<std::byte, kphp::memory::script_allocator>& response_buffer,
                    F response_handler) noexcept -> kphp::coro::task<std::expected<void, int32_t>>;
};

inline auto Client::create(std::string_view component_name) noexcept -> std::expected<Client, int32_t> {
  auto expected{kphp::component::stream::open(component_name, k2::stream_kind::component)};
  if (!expected) [[unlikely]] {
    return std::unexpected{expected.error()};
  }
  // Create and move stream into a session
  return std::expected<Client, int32_t>{Client{std::move(expected.value())}};
}

template<std::invocable<std::span<std::byte>> F>
requires std::convertible_to<kphp::coro::async_function_return_type_t<F, std::span<std::byte>>, bool>
inline auto Client::query(std::span<const std::byte> request, kphp::stl::vector<std::byte, kphp::memory::script_allocator>& response_buffer,
                          F response_handler) noexcept -> kphp::coro::task<std::expected<void, int32_t>> {
  // If previously any readers' error has been occurred
  if (reader.ctx.get()->error) [[unlikely]] {
    co_return std::unexpected(reader.ctx.get()->error.value());
  }

  kphp::log::assertion(query_count < std::numeric_limits<query_id_type>::max());
  auto query_id{query_count++};

  // Register a new query and send request
  reader.register_query(query_id, response_buffer);
  if (auto res{co_await writer.write(transport, query_id, request)}; !res) [[unlikely]] {
    co_return std::move(res);
  }

  auto still_wait{true};
  auto& response_notifier{reader.ctx.get()->query2notifier[query_id]};
  // Wait a new response until handler returns false
  while (still_wait) {
    co_await response_notifier;
    // First of all, turn off notifier
    response_notifier.unset();

    // If any readers' error has been occurred
    if (reader.ctx.get()->error) [[unlikely]] {
      co_return std::unexpected(reader.ctx.get()->error.value());
    }

    // Invoke handler and pass response slice
    still_wait = co_await std::invoke(response_handler, reader.ctx.get()->query2resp[query_id]);
  }
  co_return std::expected<void, int32_t>{};
}

} // namespace kphp::component::InterComponentSession
