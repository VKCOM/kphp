// Compiler for PHP (aka KPHP)
// Copyright (c) 2026 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <expected>
#include <memory>
#include <string_view>
#include <utility>

#include "common/rpc-error-codes.h"
#include "runtime-common/stdlib/string/string-context.h"
#include "runtime-light/coroutine/io-scheduler.h"
#include "runtime-light/coroutine/task.h"
#include "runtime-light/k2-platform/k2-api.h"
#include "runtime-light/stdlib/time/time-functions.h"

namespace kphp::rpc {

inline constexpr std::string_view EMPTY_QUERY_ERROR_DESCRIPTION = "fetching rpc response from empty query";
inline constexpr std::string_view TIMEOUT_ERROR_DESCRIPTION = "rpc response timeout";
inline constexpr std::string_view INTERNAL_ERROR_DESCRIPTION = "internal error while fetching rpc response";

class query_handle {
  k2::descriptor m_descriptor{k2::INVALID_PLATFORM_DESCRIPTOR};
  std::chrono::nanoseconds m_deadline{};

  query_handle(k2::descriptor descriptor, std::chrono::nanoseconds deadline) noexcept
      : m_descriptor{descriptor},
        m_deadline{deadline} {}

  auto get_ready_response() noexcept -> std::expected<std::span<std::byte>, std::pair<int32_t, std::string_view>>;

public:
  query_handle() = delete;

  query_handle(query_handle&& other) noexcept
      : m_descriptor{std::exchange(other.m_descriptor, k2::INVALID_PLATFORM_DESCRIPTOR)},
        m_deadline{other.m_deadline} {}

  query_handle& operator=(query_handle&& other) noexcept {
    if (this != std::addressof(other)) {
      drop();
      m_descriptor = std::exchange(other.m_descriptor, k2::INVALID_PLATFORM_DESCRIPTOR);
      m_deadline = other.m_deadline;
    }
    return *this;
  }

  ~query_handle() {
    drop();
  }

  query_handle(const query_handle& other) = delete;
  query_handle& operator=(const query_handle& other) = delete;

  static auto send(std::string_view actor, std::chrono::milliseconds timeout, std::span<const std::byte> request_buffer) noexcept
      -> std::expected<query_handle, int32_t>;

  auto wait_for_response() noexcept -> kphp::coro::task<void>;
  auto get_response() noexcept -> kphp::coro::task<std::expected<std::span<std::byte>, std::pair<int32_t, std::string_view>>>;
  auto drop() noexcept -> void;
};

inline auto query_handle::drop() noexcept -> void {
  if (m_descriptor != k2::INVALID_PLATFORM_DESCRIPTOR) {
    k2::free_descriptor(std::exchange(m_descriptor, k2::INVALID_PLATFORM_DESCRIPTOR));
  }
}

inline auto query_handle::send(std::string_view actor, std::chrono::milliseconds timeout, std::span<const std::byte> request_buffer) noexcept
    -> std::expected<query_handle, int32_t> {
  auto descriptor_exp{k2::rpc_send_request(actor, request_buffer, RpcKind::TL_RPC)};
  if (!descriptor_exp) {
    return std::unexpected{descriptor_exp.error()};
  }
  k2::descriptor descriptor{*descriptor_exp};

  std::chrono::nanoseconds deadline{kphp::time::expires_at(timeout)};

  return {query_handle{descriptor, deadline}};
}

inline auto query_handle::get_ready_response() noexcept -> std::expected<std::span<std::byte>, std::pair<int32_t, std::string_view>> {
  std::expected<size_t, int32_t> response_size_exp{k2::rpc_get_response_size(m_descriptor)};
  if (!response_size_exp) {
    return std::unexpected{std::make_pair(TL_ERROR_INTERNAL, INTERNAL_ERROR_DESCRIPTION)};
  }
  size_t response_size{*response_size_exp};

  auto& string_lib_ctx{StringLibContext::get()};
  std::span<std::byte> response_buffer{reinterpret_cast<std::byte*>(string_lib_ctx.static_buf.get()), response_size};
  std::expected<void, int32_t> response_fetch_result{k2::rpc_fetch_response(m_descriptor, response_buffer)};
  if (!response_fetch_result) {
    return std::unexpected{std::make_pair(TL_ERROR_INTERNAL, INTERNAL_ERROR_DESCRIPTION)};
  }

  return {response_buffer};
}

inline auto query_handle::wait_for_response() noexcept -> kphp::coro::task<void> {
  if (m_descriptor == k2::INVALID_PLATFORM_DESCRIPTOR) {
    co_return;
  }

  kphp::coro::io_scheduler& m_scheduler{kphp::coro::io_scheduler::get()};
  std::chrono::nanoseconds timeout{kphp::time::remaining(m_deadline)};
  if (timeout <= std::chrono::nanoseconds::zero()) {
    // if timeout <= 0 then query_handle::get_response() will return immediately without awaiting
    co_return;
  }

  // if query_handle will be destroyed after co_await start - it will not be a problem
  co_await m_scheduler.poll(m_descriptor, kphp::coro::poll_op::read, timeout);
  co_return;
}

inline auto query_handle::get_response() noexcept -> kphp::coro::task<std::expected<std::span<std::byte>, std::pair<int32_t, std::string_view>>> {
  if (m_descriptor == k2::INVALID_PLATFORM_DESCRIPTOR) {
    co_return std::unexpected{std::make_pair(TL_ERROR_INTERNAL, EMPTY_QUERY_ERROR_DESCRIPTION)};
  }

  kphp::coro::io_scheduler& m_scheduler{kphp::coro::io_scheduler::get()};
  std::chrono::nanoseconds timeout{kphp::time::remaining(m_deadline)};

  // TODO DISCUSS: may be completely remove timeout monitoring in kphp and leave it in k2-node's rpc client ???
  if (timeout <= std::chrono::nanoseconds::zero()) {
    // we cannot call m_scheduler.poll(...) with timeout <= 0, because it may hang forever
    k2::StreamStatus stream_status{};
    k2::stream_status(m_descriptor, std::addressof(stream_status));
    if (stream_status.libc_errno != k2::errno_ok) [[unlikely]] {
      co_return std::unexpected{std::make_pair(TL_ERROR_INTERNAL, INTERNAL_ERROR_DESCRIPTION)};
    }
    if (stream_status.read_status != k2::IOStatus::IOAvailable) {
      co_return std::unexpected{std::make_pair(TL_ERROR_QUERY_TIMEOUT, TIMEOUT_ERROR_DESCRIPTION)};
    }
    co_return get_ready_response();
  }

  switch (co_await m_scheduler.poll(m_descriptor, kphp::coro::poll_op::read, timeout)) {
  case kphp::coro::poll_status::event:
    co_return get_ready_response();
  case kphp::coro::poll_status::closed:
  case kphp::coro::poll_status::timeout:
    co_return std::unexpected{std::make_pair(TL_ERROR_QUERY_TIMEOUT, TIMEOUT_ERROR_DESCRIPTION)};
  case kphp::coro::poll_status::error:
    co_return std::unexpected{std::make_pair(TL_ERROR_INTERNAL, INTERNAL_ERROR_DESCRIPTION)};
  }
}

} // namespace kphp::rpc
