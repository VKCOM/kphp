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
#include "runtime-common/core/runtime-core.h"
#include "runtime-light/coroutine/io-scheduler.h"
#include "runtime-light/coroutine/task.h"
#include "runtime-light/k2-platform/k2-api.h"
#include "runtime-light/stdlib/diagnostics/logs.h"
#include "runtime-light/stdlib/time/time-functions.h"

namespace kphp::rpc {

class query_handle {
  k2::descriptor m_descriptor{k2::INVALID_PLATFORM_DESCRIPTOR};
  int64_t m_id;
  std::chrono::nanoseconds m_deadline{};
  bool m_collect_responses_extra_info{false};

  query_handle(k2::descriptor rpc_d, int64_t id, std::chrono::nanoseconds deadline, bool collect_responses_extra_info) noexcept
      : m_descriptor{rpc_d},
        m_id{id},
        m_deadline{deadline},
        m_collect_responses_extra_info{collect_responses_extra_info} {}

  auto get_ready_response() noexcept -> std::expected<string, std::pair<int32_t, string>>;
  auto drop() noexcept -> void;

public:
  query_handle() = delete;

  query_handle(query_handle&& other) noexcept
      : m_descriptor{std::exchange(other.m_descriptor, k2::INVALID_PLATFORM_DESCRIPTOR)},
        m_id{std::exchange(other.m_id, INVALID_QUERY_ID)},
        m_deadline{other.m_deadline},
        m_collect_responses_extra_info{other.m_collect_responses_extra_info} {}

  query_handle& operator=(query_handle&& other) noexcept {
    if (this != std::addressof(other)) {
      drop();
      m_descriptor = std::exchange(other.m_descriptor, k2::INVALID_PLATFORM_DESCRIPTOR);
      m_id = std::exchange(other.m_id, INVALID_QUERY_ID);
      m_deadline = other.m_deadline;
      m_collect_responses_extra_info = other.m_collect_responses_extra_info;
    }
    return *this;
  }

  ~query_handle() {
    drop();
  }

  query_handle(const query_handle& other) = delete;
  query_handle& operator=(const query_handle& other) = delete;

  static auto send(std::string_view actor, bool collect_responses_extra_info, bool ignore_answer, std::chrono::milliseconds timeout, double timestamp,
                   int64_t query_id, std::span<const std::byte> request_buffer) noexcept -> std::expected<query_handle, int32_t>;

  auto wait_for_response() noexcept -> kphp::coro::task<void>;
  auto get_response() noexcept -> kphp::coro::task<std::expected<string, std::pair<int32_t, string>>>;
};

inline auto query_handle::drop() noexcept -> void {
  if (m_descriptor != k2::INVALID_PLATFORM_DESCRIPTOR) {
    k2::free_descriptor(std::exchange(m_descriptor, k2::INVALID_PLATFORM_DESCRIPTOR));
  }
}

inline auto query_handle::wait_for_response() noexcept -> kphp::coro::task<void> {
  if (m_descriptor == k2::INVALID_PLATFORM_DESCRIPTOR) {
    co_return;
  }

  kphp::coro::io_scheduler& m_scheduler{kphp::coro::io_scheduler::get()};
  std::chrono::nanoseconds timeout{kphp::time::remaining(m_deadline)};
  if (timeout.count() <= 0) {
    co_return;
  }

  // if query_handle will be destroyed after co_await start - it will not be a problem
  co_await m_scheduler.poll(m_descriptor, kphp::coro::poll_op::read, timeout);
  co_return;
}

inline auto query_handle::get_response() noexcept -> kphp::coro::task<std::expected<string, std::pair<int32_t, string>>> {
  if (m_descriptor == k2::INVALID_PLATFORM_DESCRIPTOR) {
    co_return std::unexpected{std::make_pair(TL_ERROR_INTERNAL, string{"fetching rpc response from empty handle"})};
  }

  kphp::coro::io_scheduler& m_scheduler{kphp::coro::io_scheduler::get()};
  std::chrono::nanoseconds timeout{kphp::time::remaining(m_deadline)};
  if (timeout.count() <= 0) {
    co_return std::unexpected{std::make_pair(TL_ERROR_QUERY_TIMEOUT, string{"rpc response timeout"})};
  }

  switch (co_await m_scheduler.poll(m_descriptor, kphp::coro::poll_op::read, timeout)) {
  case kphp::coro::poll_status::event:
    co_return get_ready_response();
  case kphp::coro::poll_status::closed:
  case kphp::coro::poll_status::timeout:
    co_return std::unexpected{std::make_pair(TL_ERROR_QUERY_TIMEOUT, string{"rpc response timeout"})};
  case kphp::coro::poll_status::error:
    co_return std::unexpected{std::make_pair(TL_ERROR_INTERNAL, string{"error fetching rpc response"})};
  }
}

} // namespace kphp::rpc
