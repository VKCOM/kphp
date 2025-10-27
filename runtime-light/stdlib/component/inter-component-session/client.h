// Compiler for PHP (aka KPHP)
// Copyright (c) 2025 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cstddef>
#include <cstdint>
#include <expected>
#include <iterator>
#include <memory>
#include <optional>
#include <span>
#include <string_view>
#include <utility>

#include "runtime-light/coroutine/event.h"
#include "runtime-light/coroutine/task.h"
#include "runtime-light/stdlib/component/inter-component-session/details/function-wrapper.h"
#include "runtime-light/stdlib/diagnostics/logs.h"
#include "runtime-light/streams/stream.h"
#include "runtime-light/tl/tl-core.h"
#include "runtime-light/tl/tl-types.h"

namespace kphp::component::inter_component_session {

// Inter component client for interactions over a stream in a client-server manner
class client final {
  using query_id_type = uint64_t;
  using query2notifier_type = kphp::stl::unordered_map<query_id_type, kphp::coro::event, kphp::memory::script_allocator>;

  struct refcountable_transport_type : public refcountable_php_classes<refcountable_transport_type> {
    kphp::component::stream stream;
  };
  using shared_transport_type = class_instance<refcountable_transport_type>;

  shared_transport_type transport;
  query_id_type query_count{};

  // ======== WRITER ========
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
      is_occupied = true;
      occupied_by = qid;

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

  // ======== READER ========
  struct reader {
    struct refcountable_ctx_type : public refcountable_php_classes<refcountable_ctx_type> {
      query2notifier_type query2notifier{};
      kphp::stl::unordered_map<query_id_type, details::function_wrapper<std::span<std::byte>, size_t>, kphp::memory::script_allocator>
          query2resp_buffer_provider{};
      kphp::stl::unordered_map<query_id_type, std::span<std::byte>, kphp::memory::script_allocator> query2resp{};
      kphp::coro::event interrupted{};
      std::optional<int32_t> error{};
    };
    using shared_ctx_type = class_instance<refcountable_ctx_type>;
    using interrupter_type = kphp::coro::shared_task<int>; // FIXME: remove `int` as return type after `when_any` fixing

    // The following reader state is intended to be initialized once for a new client.
    // It is assumed that "copying" (ref count increasing) will be the common case, rather than moving.
    shared_ctx_type ctx;
    interrupter_type interrupter;
    kphp::coro::shared_task<void> runner;

    reader(const shared_ctx_type& _ctx, shared_transport_type _t) noexcept
        : ctx(_ctx),
          interrupter(reader::wait_until_interrupt(_ctx)),
          runner(reader::run(_ctx, std::move(_t), interrupter)){};

    static inline auto run(shared_ctx_type ctx, shared_transport_type t, interrupter_type interrupter) noexcept -> kphp::coro::shared_task<void> {
      // Allocate buffer for header
      tl::InterComponentSessionResponseHeader resp_header{};
      std::array<std::byte, resp_header.footprint()> resp_header_buf{};

      while (!ctx.get()->interrupted.is_set()) {
        // Read response header or interrupt
        auto read_header_res{co_await kphp::coro::when_any(t.get()->stream.read(resp_header_buf), interrupter)};
        // Interrupt is happened
        if (std::holds_alternative<int>(read_header_res)) [[unlikely]] { // FIXME: remove `int` after `when_any` fixing
          kphp::log::debug("reader has been interrupted");
          break;
        } else if (auto res{std::get<std::expected<size_t, int32_t>>(read_header_res)}; !res) [[unlikely]] {
          kphp::log::warning("an error occurred while reading the header from a stream: {}", res.error());
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
        kphp::log::debug("read {} bytes for query #{} ", size, qid);

        // Ensure that buffer for response can be provided
        kphp::log::assertion(ctx.get()->query2resp_buffer_provider.contains(qid));

        // Make an appropriate buffer's slice for a response
        auto resp{ctx.get()->query2resp_buffer_provider.at(qid)(size)};
        ctx.get()->query2resp[qid] = resp;

        // Read payload
        if (auto res{co_await t.get()->stream.read(resp)}; !res) [[unlikely]] {
          kphp::log::warning("an error occurred while reading the payload from a stream: {}", res.error());
          ctx.get()->error = res.error();
          break;
        }
        kphp::log::debug("resp buffer first byte: {} Resp buffer last byte: {} ", static_cast<uint8_t>(*std::next(resp.begin(), 0)),
                         static_cast<uint8_t>(*std::next(resp.begin(), resp.size() - 1)));

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

    // Dummy routine for waiting until an interrupting (stopping) event will happen
    static inline auto wait_until_interrupt(shared_ctx_type ctx) noexcept -> interrupter_type {
      co_await ctx.get()->interrupted;
      co_return 0;
    }

    // Semantics of this method is considering tha state will be changed. That's why it is not marked as `const`
    inline auto register_query(query_id_type qid, details::function_wrapper<std::span<std::byte>, size_t>&& buffer_provider) noexcept -> void {
      // We wouldn't read a response twice
      kphp::log::assertion(ctx.get()->query2notifier.contains(qid) == false);

      // Register provider of storage for a response
      ctx.get()->query2resp_buffer_provider.insert({qid, std::move(buffer_provider)});

      // Register notifier
      ctx.get()->query2notifier.emplace(qid, kphp::coro::event{});
    }
  } reader;

  explicit client(kphp::component::stream&& s) noexcept
      : transport(make_instance<refcountable_transport_type>(refcountable_transport_type{.stream = std::move(s)})),
        reader({make_instance<reader::refcountable_ctx_type>(), transport}) {
    auto& scheduler{kphp::coro::io_scheduler::get()};
    // Interrupter needs for immediately stopping the reader in the of client's life
    scheduler.start(reader.interrupter);
    // Run reader as "service"
    scheduler.start(reader.runner);
  }

public:
  ~client() noexcept {
    // If client has been moved, skip disabling the reader.
    // Otherwise, shut down the reader.
    if (query_count != std::numeric_limits<query_id_type>::max()) {
      reader.ctx.get()->interrupted.set();
    }
  }

  // Designed that `transport` and `reader` will be allocated once as refcountable class instance.
  // Moving looks like copying but is simply reference count increasing for 'transport' and 'reader' fields.
  // Such approach motivated by the fact that the "reader-service" cannot be easily moved due to depends on transport and cannot be trivial stopped.
  client(client&& other) noexcept
      : transport(other.transport), // Intentionally call of copy constructor for shared transport
        query_count(std::exchange(other.query_count, std::numeric_limits<query_id_type>::max())),
        writer(std::move(other.writer)),
        reader(other.reader) /* Intentionally call of copy constructor for shared reader  */ {}

  auto operator=(client&& other) noexcept -> client& {
    if (this != std::addressof(other)) {
      transport = other.transport; // Intentionally call of copy assignment for shared transport
      query_count = std::exchange(other.query_count, std::numeric_limits<query_id_type>::max());
      writer = std::move(other.writer);
      reader = other.reader; // Intentionally call of copy assignment for shared reader
    }
    return *this;
  }

  client(const client&) = delete;
  auto operator=(const client&) -> client& = delete;

  static inline auto create(std::string_view component_name) noexcept -> std::expected<client, int32_t>;

  template<std::invocable<size_t> B, std::invocable<std::span<std::byte>> R>
  requires std::is_convertible_v<std::invoke_result_t<B, size_t>, std::span<std::byte>> &&
               std::is_convertible_v<std::invoke_result_t<R, std::span<std::byte>>, bool>
  inline auto query(std::span<const std::byte> request, B&& response_buffer_provider,
                    R response_handler) noexcept -> kphp::coro::task<std::expected<void, int32_t>>;

  template<std::invocable<size_t> B>
  requires std::is_convertible_v<std::invoke_result_t<B, size_t>, std::span<std::byte>>
  inline auto query(std::span<const std::byte> request,
                    B&& response_buffer_provider) noexcept -> kphp::coro::task<std::expected<std::span<std::byte>, int32_t>>;
};

inline auto client::create(std::string_view component_name) noexcept -> std::expected<client, int32_t> {
  auto expected{kphp::component::stream::open(component_name, k2::stream_kind::component)};
  if (!expected) [[unlikely]] {
    return std::unexpected{expected.error()};
  }
  // Create and move stream into a session
  return std::expected<client, int32_t>{client{std::move(expected.value())}};
}

template<std::invocable<size_t> B, std::invocable<std::span<std::byte>> R>
requires std::is_convertible_v<std::invoke_result_t<B, size_t>, std::span<std::byte>> &&
             std::is_convertible_v<std::invoke_result_t<R, std::span<std::byte>>, bool>
inline auto client::query(std::span<const std::byte> request, B&& response_buffer_provider,
                          R response_handler) noexcept -> kphp::coro::task<std::expected<void, int32_t>> {
  // If previously any readers' error has been occurred
  if (reader.ctx.get() == nullptr) [[unlikely]] {
    co_return std::unexpected(k2::errno_eshutdown);
  }

  kphp::log::assertion(query_count < std::numeric_limits<query_id_type>::max());
  auto query_id{query_count++};

  // Register a new query and send request
  reader.register_query(query_id, details::function_wrapper<std::span<std::byte>, size_t>{std::forward<B>(response_buffer_provider)});
  kphp::log::debug("client create query #{}", query_id);
  if (auto res{co_await writer.write(transport, query_id, request)}; !res) [[unlikely]] {
    co_return std::move(res);
  }
  kphp::log::debug("client wrote request for query #{}", query_id);

  auto still_wait{true};
  auto& response_notifier{reader.ctx.get()->query2notifier[query_id]};

  kphp::log::debug("client now is reading responses for query #{}", query_id);
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
    still_wait = std::invoke(std::forward<R>(response_handler), reader.ctx.get()->query2resp[query_id]);
  }
  co_return std::expected<void, int32_t>{};
}

template<std::invocable<size_t> B>
requires std::is_convertible_v<std::invoke_result_t<B, size_t>, std::span<std::byte>>
inline auto client::query(std::span<const std::byte> request,
                          B&& response_buffer_provider) noexcept -> kphp::coro::task<std::expected<std::span<std::byte>, int32_t>> {
  std::span<std::byte> response{};
  auto res{co_await query(request, std::forward<B>(response_buffer_provider), [&response](std::span<std::byte> resp) noexcept -> bool {
    response = resp;
    return false;
  })};
  if (!res.has_value()) [[unlikely]] {
    co_return std::unexpected{res.error()};
  }
  co_return std::expected<std::span<std::byte>, int32_t>{response};
}

} // namespace kphp::component::inter_component_session
