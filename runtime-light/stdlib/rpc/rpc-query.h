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

class query {
  k2::descriptor m_descriptor{k2::INVALID_PLATFORM_DESCRIPTOR};
  std::chrono::steady_clock::time_point m_deadline;

  query(k2::descriptor descriptor, std::chrono::steady_clock::time_point deadline) noexcept
      : m_descriptor{descriptor},
        m_deadline{deadline} {}

  template<typename ResponseAllocator>
  requires std::invocable<ResponseAllocator, size_t> && std::is_same_v<std::invoke_result_t<ResponseAllocator, size_t>, std::span<std::byte>>
  auto get_ready_response(ResponseAllocator response_allocator) noexcept -> std::expected<std::span<std::byte>, int32_t>;

  auto drop() noexcept -> void;

public:
  query() = delete;

  query(query&& other) noexcept
      : m_descriptor{std::exchange(other.m_descriptor, k2::INVALID_PLATFORM_DESCRIPTOR)},
        m_deadline{other.m_deadline} {}

  query& operator=(query&& other) noexcept {
    if (this != std::addressof(other)) {
      drop();
      m_descriptor = std::exchange(other.m_descriptor, k2::INVALID_PLATFORM_DESCRIPTOR);
      m_deadline = other.m_deadline;
    }
    return *this;
  }

  ~query() {
    drop();
  }

  query(const query& other) = delete;
  query& operator=(const query& other) = delete;

  static auto send(std::string_view actor, std::chrono::milliseconds timeout, std::span<const std::byte> request_buffer) noexcept
      -> std::expected<query, int32_t>;

  auto wait_for_response() noexcept -> kphp::coro::task<void>;

  template<typename ResponseAllocator>
  requires std::invocable<ResponseAllocator, size_t> && std::is_same_v<std::invoke_result_t<ResponseAllocator, size_t>, std::span<std::byte>>
  auto get_response(ResponseAllocator response_allocator) noexcept -> kphp::coro::task<std::expected<std::span<std::byte>, int32_t>>;
};

inline auto query::drop() noexcept -> void {
  if (m_descriptor != k2::INVALID_PLATFORM_DESCRIPTOR) {
    k2::free_descriptor(std::exchange(m_descriptor, k2::INVALID_PLATFORM_DESCRIPTOR));
  }
}

inline auto query::send(std::string_view actor, std::chrono::milliseconds timeout, std::span<const std::byte> request_buffer) noexcept
    -> std::expected<query, int32_t> {
  auto descriptor_exp{k2::rpc_send_request(actor, request_buffer, RpcKind::TL_RPC)};
  if (!descriptor_exp) {
    return std::unexpected{descriptor_exp.error()};
  }
  k2::descriptor descriptor{*descriptor_exp};

  auto deadline{kphp::time::expires_at(std::chrono::duration_cast<std::chrono::nanoseconds>(timeout))};

  return {query{descriptor, deadline}};
}

template<typename ResponseAllocator>
requires std::invocable<ResponseAllocator, size_t> && std::is_same_v<std::invoke_result_t<ResponseAllocator, size_t>, std::span<std::byte>>
auto query::get_ready_response(ResponseAllocator response_allocator) noexcept -> std::expected<std::span<std::byte>, int32_t> {
  std::expected<size_t, int32_t> response_size_exp{k2::rpc_get_response_size(m_descriptor)};
  if (!response_size_exp) {
    switch (response_size_exp.error()) {
    case k2::errno_eagain:
      return std::unexpected{TL_ERROR_QUERY_TIMEOUT};
    default:
      return std::unexpected{TL_ERROR_INTERNAL};
    }
  }
  size_t response_size{*response_size_exp};

  std::span<std::byte> response_buffer{std::invoke(response_allocator, response_size)};
  std::expected<void, int32_t> response_fetch_result{k2::rpc_fetch_response(m_descriptor, response_buffer)};
  if (!response_fetch_result) {
  // TODO deallocate response_buffer
    return std::unexpected{TL_ERROR_INTERNAL};
  }

  return {response_buffer};
}

inline auto query::wait_for_response() noexcept -> kphp::coro::task<void> {
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

template<typename ResponseAllocator>
requires std::invocable<ResponseAllocator, size_t> && std::is_same_v<std::invoke_result_t<ResponseAllocator, size_t>, std::span<std::byte>>
inline auto query::get_response(ResponseAllocator response_allocator) noexcept -> kphp::coro::task<std::expected<std::span<std::byte>, int32_t>> {
  if (m_descriptor == k2::INVALID_PLATFORM_DESCRIPTOR) {
    co_return std::unexpected{TL_ERROR_INTERNAL};
  }

  kphp::coro::io_scheduler& m_scheduler{kphp::coro::io_scheduler::get()};
  std::chrono::nanoseconds timeout{kphp::time::remaining(m_deadline)};

  // TODO DISCUSS: may be completely remove timeout monitoring in kphp and leave it in k2-node's rpc client ???
  if (timeout <= std::chrono::nanoseconds::zero()) {
    co_return get_ready_response(response_allocator);
  }

  switch (co_await m_scheduler.poll(m_descriptor, kphp::coro::poll_op::read, timeout)) {
  case kphp::coro::poll_status::event:
    co_return get_ready_response(response_allocator);
  case kphp::coro::poll_status::closed:
  case kphp::coro::poll_status::timeout:
    co_return std::unexpected{TL_ERROR_QUERY_TIMEOUT};
  case kphp::coro::poll_status::error:
    co_return std::unexpected{TL_ERROR_INTERNAL};
  }
}

} // namespace kphp::rpc
