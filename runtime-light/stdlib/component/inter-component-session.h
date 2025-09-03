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
#include "runtime-light/coroutine/io-scheduler.h"
#include "runtime-light/coroutine/poll.h"
#include "runtime-light/coroutine/task.h"
#include "runtime-light/k2-platform/k2-api.h"
#include "runtime-light/stdlib/diagnostics/logs.h"
#include "runtime-light/streams/stream.h"
#include "runtime-light/tl/tl-core.h"
#include "runtime-light/tl/tl-functions.h"
#include "runtime-light/tl/tl-types.h"

namespace kphp::component {

class InterComponentSession {
  using query_id_type = uint64_t;
  using query2notifier_type = kphp::stl::unordered_map<query_id_type, kphp::coro::event, kphp::memory::script_allocator>;

  struct refcountable_transport_type : public refcountable_php_classes<refcountable_transport_type> {
    kphp::component::stream stream;
    explicit refcountable_transport_type(kphp::component::stream&& s) noexcept
        : stream(std::move(s)){};
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
        query2notifier[qid] = {};
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
          query2resp_storage{};
      bool is_running{true};
    };
    using shared_ctx_type = class_instance<refcountable_ctx_type>;

    // The following state is intended to be initialized once for a new session.
    // It is assumed that copying will be the common case, rather than moving.
    shared_ctx_type ctx;
    kphp::coro::shared_task<std::expected<void, int32_t>> runner;

    reader(shared_ctx_type _ctx, shared_transport_type t) noexcept
        : ctx(std::move(_ctx)),
          runner(reader::run(std::move(ctx), std::move(t))){};

    static inline auto run(shared_ctx_type&& ctx, shared_transport_type&& t) noexcept -> kphp::coro::shared_task<std::expected<void, int32_t>> {
      // Prepare buffer for header
      tl::InterComponentSessionResponseHeader resp_header{};
      std::array<std::byte, resp_header.footprint()> resp_header_buf{};

      while (ctx.get()->is_running) {
        // Read response header
        if (auto res{co_await t.get()->stream.read(resp_header_buf)}; !res) [[unlikely]] {
          co_return std::unexpected{res.error()};
        }

        // Parse response id and size
        tl::fetcher tlf{resp_header_buf};
        if (!resp_header.fetch(tlf)) [[unlikely]] {
          co_return std::unexpected(k2::errno_einval);
        }

        // Ensure that buffer for response is presented
        auto qid{resp_header.id.value};
        auto size{resp_header.size.value};
        kphp::log::assertion(ctx.get()->query2resp_storage.contains(qid));

        // Reserve enough space for response
        ctx.get()->query2resp_storage.at(qid).reserve(size);

        // Read payload
        if (auto res{co_await t.get()->stream.read({ctx.get()->query2resp_storage.at(qid).data(), size})}; !res) [[unlikely]] {
          co_return std::unexpected{res.error()};
        }

        // Ensure that notifier is presented and notify
        kphp::log::assertion(ctx.get()->query2notifier.contains(qid));
        ctx.get()->query2notifier[qid].set();
      }
      co_return std::expected<void, int32_t>{};
    }

    inline auto register_query(query_id_type qid, kphp::stl::vector<std::byte, kphp::memory::script_allocator>& storage) noexcept -> kphp::coro::event {
      // We wouldn't read a response twice
      kphp::log::assertion(ctx.get()->query2notifier.contains(qid) == false);

      // Register storage for a response
      ctx.get()->query2resp_storage.at(qid) = storage;

      // Register notifier
      ctx.get()->query2notifier[qid] = {};
      return ctx.get()->query2notifier[qid];
    }
  } reader;

  explicit InterComponentSession(kphp::component::stream&& s)
      : transport(make_instance<refcountable_transport_type>(std::move(s))),
        reader({make_instance<reader::refcountable_ctx_type>(), transport}) {
    kphp::coro::io_scheduler::get().start(reader.runner);
  }

public:
  ~InterComponentSession() {
    // If the session has been moved, skip disabling the reader.
    // Otherwise, shut down the reader.
    if (query_count != std::numeric_limits<query_id_type>::max()) {
      reader.ctx.get()->is_running = false;
    }
  }

  InterComponentSession(InterComponentSession&& other) noexcept
      : transport(other.transport), // Intentionally call of copy constructor for shared transport
        query_count(std::exchange(other.query_count, std::numeric_limits<query_id_type>::max())),
        writer(std::move(other.writer)),
        reader(other.reader) /* Intentionally call of copy constructor for shared reader  */ {}

  auto operator=(InterComponentSession&& other) noexcept -> InterComponentSession& {
    if (this != std::addressof(other)) {
      transport = other.transport; // Intentionally call of copy assignment for shared transport
      query_count = std::exchange(other.query_count, std::numeric_limits<query_id_type>::max());
      writer = std::move(other.writer);
      reader = other.reader; // Intentionally call of copy assignment for shared transport
    }
    return *this;
  }

  InterComponentSession(const InterComponentSession&) = delete;
  auto operator=(const InterComponentSession&) -> InterComponentSession& = delete;

  static inline auto create(std::string_view component_name) noexcept -> std::expected<InterComponentSession, int32_t>;

  inline auto query(std::span<const std::byte> request,
                    kphp::stl::vector<std::byte, kphp::memory::script_allocator>& response) noexcept -> kphp::coro::task<std::expected<void, int32_t>>;
};

inline auto InterComponentSession::create(std::string_view component_name) noexcept -> std::expected<InterComponentSession, int32_t> {
  auto expected{kphp::component::stream::open(component_name, k2::stream_kind::component)};
  if (!expected) [[unlikely]] {
    return std::unexpected{expected.error()};
  }
  // Create and move stream into session
  return std::expected<InterComponentSession, int32_t>{InterComponentSession{std::move(expected.value())}};
}

inline auto InterComponentSession::query(std::span<const std::byte> request, kphp::stl::vector<std::byte, kphp::memory::script_allocator>& response) noexcept
    -> kphp::coro::task<std::expected<void, int32_t>> {
  kphp::log::assertion(query_count < std::numeric_limits<query_id_type>::max());
  auto query_id{query_count++};

  auto response_awaiter{reader.register_query(query_id, response)};
  if (auto res{co_await writer.write(transport, query_id, request)}; !res) {
    co_return std::move(res);
  }
  co_await response_awaiter;
  co_return std::expected<void, int32_t>{};
}

} // namespace kphp::component
