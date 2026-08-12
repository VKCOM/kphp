// Compiler for PHP (aka KPHP)
// Copyright (c) 2026 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <chrono>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <expected>
#include <functional>
#include <memory>
#include <span>
#include <string_view>
#include <type_traits>
#include <utility>

#include "common/rpc-error-codes.h"
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

  static auto send(std::string_view actor, std::chrono::milliseconds timeout, std::span<const std::byte> request_buffer,
                   k2::RpcKind rpc_kind) noexcept -> std::expected<query, int32_t>;

  template<std::invocable<size_t> B>
  requires std::is_same_v<std::invoke_result_t<B, size_t>, std::span<std::byte>>
  auto response(B response_buffer_provider) && noexcept -> kphp::coro::task<std::expected<std::span<std::byte>, int32_t>>;
};

inline auto query::drop() noexcept -> void {
  if (m_descriptor != k2::INVALID_PLATFORM_DESCRIPTOR) {
    k2::free_descriptor(std::exchange(m_descriptor, k2::INVALID_PLATFORM_DESCRIPTOR));
  }
}

inline auto query::send(std::string_view actor, std::chrono::milliseconds timeout, std::span<const std::byte> request_buffer,
                        k2::RpcKind rpc_kind) noexcept -> std::expected<query, int32_t> {
  auto descriptor_exp{k2::rpc_send_request(actor, request_buffer, rpc_kind)};
  if (!descriptor_exp) {
    return std::unexpected{descriptor_exp.error()};
  }
  k2::descriptor descriptor{*descriptor_exp};

  auto deadline{kphp::time::expires_at(timeout)};

  return {query{descriptor, deadline}};
}

template<std::invocable<size_t> B>
requires std::is_same_v<std::invoke_result_t<B, size_t>, std::span<std::byte>>
auto query::response(B response_buffer_provider) && noexcept -> kphp::coro::task<std::expected<std::span<std::byte>, int32_t>> {

  static constexpr auto get_ready_response{
      [](k2::descriptor& descriptor, B&& response_buffer_provider) noexcept -> std::expected<std::span<std::byte>, int32_t> {
        std::expected<size_t, int32_t> response_size_exp{k2::rpc_get_response_size(descriptor)};
        if (!response_size_exp) {
          switch (response_size_exp.error()) {
          case k2::errno_eagain:
            return std::unexpected{TL_ERROR_QUERY_TIMEOUT};
          default:
            return std::unexpected{TL_ERROR_INTERNAL};
          }
        }
        size_t response_size{*response_size_exp};

        std::span<std::byte> response_buffer{std::invoke(std::forward<B>(response_buffer_provider), response_size)};
        if (response_buffer.size() < response_size) {
          return std::unexpected{TL_ERROR_RESULT_TOO_LARGE};
        }
        std::expected<void, int32_t> response_fetch_result{k2::rpc_fetch_response(descriptor, response_buffer)};
        if (!response_fetch_result) {
          return std::unexpected{TL_ERROR_INTERNAL};
        }

        return {response_buffer};
      }};

  if (m_descriptor == k2::INVALID_PLATFORM_DESCRIPTOR) {
    co_return std::unexpected{TL_ERROR_INVALID_CONNECTION_ID};
  }

  auto timeout{kphp::time::remaining(m_deadline)};
  if (timeout <= std::chrono::nanoseconds::zero()) {
    co_return get_ready_response(m_descriptor, std::move(response_buffer_provider));
  }

  auto m_descriptor_copy{std::exchange(m_descriptor, k2::INVALID_PLATFORM_DESCRIPTOR)};
  switch (co_await kphp::coro::io_scheduler::get().poll(m_descriptor_copy, kphp::coro::poll_op::read, timeout)) {
  case kphp::coro::poll_status::event:
    co_return get_ready_response(m_descriptor_copy, std::move(response_buffer_provider));
  case kphp::coro::poll_status::timeout:
    co_return std::unexpected{TL_ERROR_QUERY_TIMEOUT};
  case kphp::coro::poll_status::closed:
  case kphp::coro::poll_status::error:
    co_return std::unexpected{TL_ERROR_INTERNAL};
  }
}
} // namespace kphp::rpc
