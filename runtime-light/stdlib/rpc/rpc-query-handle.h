// Compiler for PHP (aka KPHP)
// Copyright (c) 2026 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <expected>
#include <memory>
#include <tuple>
#include <utility>

#include "common/rpc-error-codes.h"
#include "rpc-client-state.h"
#include "runtime-common/core/runtime-core.h"
#include "runtime-light/coroutine/io-scheduler.h"
#include "runtime-light/coroutine/task.h"
#include "runtime-light/k2-platform/k2-api.h"
#include "runtime-light/stdlib/diagnostics/logs.h"
#include "runtime-light/stdlib/rpc/rpc-client-state.h"
#include "runtime-light/stdlib/time/util.h"

namespace kphp::rpc {

class query_handle {
  k2::descriptor rpc_d{k2::INVALID_PLATFORM_DESCRIPTOR};
  int64_t query_id;
  std::chrono::nanoseconds deadline{};
  bool collect_responses_extra_info{false};

public:
  query_handle() = delete;

  query_handle(k2::descriptor _rpc_d, int64_t _query_id, std::chrono::nanoseconds _deadline, bool _collect_responses_extra_info) noexcept
      : rpc_d{_rpc_d},
        query_id{_query_id},
        deadline{_deadline},
        collect_responses_extra_info{_collect_responses_extra_info} {}

  query_handle(query_handle&& other) noexcept
      : rpc_d{std::exchange(other.rpc_d, k2::INVALID_PLATFORM_DESCRIPTOR)},
        query_id{other.query_id},
        deadline{other.deadline},
        collect_responses_extra_info{other.collect_responses_extra_info} {}

  query_handle& operator=(query_handle&& other) noexcept {
    if (this != std::addressof(other)) {
      drop();
      rpc_d = std::exchange(other.rpc_d, k2::INVALID_PLATFORM_DESCRIPTOR);
      query_id = other.query_id;
      deadline = other.deadline;
      collect_responses_extra_info = other.collect_responses_extra_info;
    }
    return *this;
  }

  query_handle(const query_handle& other) = delete;

  query_handle& operator=(const query_handle& other) = delete;

  ~query_handle() noexcept {
    drop();
  }

  void drop() noexcept {
    if (rpc_d != k2::INVALID_PLATFORM_DESCRIPTOR) {
      k2::free_descriptor(std::exchange(rpc_d, k2::INVALID_PLATFORM_DESCRIPTOR));
    }
  }

  kphp::coro::task<std::expected<string, std::pair<int32_t, string>>> get_response() noexcept {
    if (rpc_d == k2::INVALID_PLATFORM_DESCRIPTOR) {
      co_return std::unexpected{std::make_pair(TL_ERROR_INTERNAL, string{"fetching rpc response from empty handle"})};
    }

    kphp::coro::io_scheduler& m_scheduler{kphp::coro::io_scheduler::get()};
    std::chrono::nanoseconds timeout{deadline_to_timeout(deadline)};

    switch (co_await m_scheduler.poll(rpc_d, kphp::coro::poll_op::read, timeout)) {
    case kphp::coro::poll_status::event:
      co_return get_ready_response();
    case kphp::coro::poll_status::closed:
    case kphp::coro::poll_status::timeout:
      co_return std::unexpected{std::make_pair(TL_ERROR_QUERY_TIMEOUT, string{"rpc response timeout"})};
    case kphp::coro::poll_status::error:
      co_return std::unexpected{std::make_pair(TL_ERROR_INTERNAL, string{"error fetching rpc response"})};
    }
  }

private:
  std::expected<string, std::pair<int32_t, string>> get_ready_response() noexcept;
};

std::expected<query_handle, int32_t> send_and_get_handle(std::string_view actor, bool collect_responses_extra_info, bool ignore_answer,
                                                         std::chrono::milliseconds timeout, double timestamp, int64_t query_id,
                                                         std::span<const std::byte> request_buffer) noexcept;

} // namespace kphp::rpc
