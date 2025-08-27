// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <concepts>
#include <cstddef>
#include <cstdint>
#include <expected>
#include <span>
#include <string_view>
#include <utility>

#include "common/algorithms/hashes.h"
#include "runtime-common/core/class-instance/refcountable-php-classes.h"
#include "runtime-common/core/runtime-core.h"
#include "runtime-light/coroutine/task.h"
#include "runtime-light/k2-platform/k2-api.h"
#include "runtime-light/stdlib/diagnostics/logs.h"
#include "runtime-light/stdlib/fork/fork-functions.h"
#include "runtime-light/streams/read-ext.h"
#include "runtime-light/streams/stream.h"

namespace kphp::component {

inline auto send_request(kphp::component::stream& stream, std::span<const std::byte> request) noexcept -> kphp::coro::task<std::expected<void, int32_t>> {
  if (auto expected{co_await stream.write_all(request)}; !expected) [[unlikely]] {
    co_return std::move(expected);
  }
  stream.shutdown_write();
  co_return std::expected<void, int32_t>{};
}

inline auto fetch_response(const kphp::component::stream& stream, std::span<std::byte> response) noexcept -> kphp::coro::task<std::expected<size_t, int32_t>> {
  co_return co_await stream.read(response);
}

template<std::invocable<std::span<const std::byte>> F>
auto fetch_response(const kphp::component::stream& stream, F f) noexcept -> kphp::coro::task<std::expected<void, int32_t>> {
  co_return co_await stream.read_all(std::move(f));
}

inline auto fetch_request(const kphp::component::stream& stream, std::span<std::byte> request) noexcept -> kphp::coro::task<std::expected<size_t, int32_t>> {
  co_return co_await stream.read(request);
}

template<std::invocable<std::span<const std::byte>> F>
auto fetch_request(const kphp::component::stream& stream, F f) noexcept -> kphp::coro::task<std::expected<void, int32_t>> {
  co_return co_await stream.read_all(std::move(f));
}

inline auto send_response(kphp::component::stream& stream, std::span<const std::byte> response) noexcept -> kphp::coro::task<std::expected<void, int32_t>> {
  if (auto expected{co_await stream.write_all(response)}; !expected) [[unlikely]] {
    co_return std::move(expected);
  }
  stream.shutdown_write();
  co_return std::expected<void, int32_t>{};
}

inline auto query(kphp::component::stream& stream, std::span<const std::byte> request, std::span<std::byte> response) noexcept
    -> kphp::coro::task<std::expected<size_t, int32_t>> {
  if (auto expected{co_await send_request(stream, request)}; !expected) [[unlikely]] {
    co_return std::unexpected{expected.error()};
  }
  co_return co_await fetch_response(stream, response);
}

template<std::invocable<std::span<const std::byte>> F>
auto query(kphp::component::stream& stream, std::span<const std::byte> request, F f) noexcept -> kphp::coro::task<std::expected<void, int32_t>> {
  if (auto expected{co_await send_request(stream, request)}; !expected) [[unlikely]] {
    co_return std::move(expected);
  }
  if (auto expected{co_await fetch_response(stream, std::move(f))}; !expected) [[unlikely]] {
    co_return std::move(expected);
  }
  co_return std::expected<void, int32_t>{};
}

} // namespace kphp::component

// === ComponentQuery =============================================================================

class C$ComponentQuery final : public refcountable_php_classes<C$ComponentQuery> {
  kphp::component::stream m_stream;

public:
  explicit C$ComponentQuery(kphp::component::stream stream) noexcept
      : m_stream(std::move(stream)) {}
  C$ComponentQuery(C$ComponentQuery&& other) noexcept
      : m_stream(std::move(other.m_stream)) {}
  ~C$ComponentQuery() = default;

  C$ComponentQuery(const C$ComponentQuery&) = delete;
  C$ComponentQuery& operator=(const C$ComponentQuery&) = delete;
  C$ComponentQuery& operator=(C$ComponentQuery&& other) = delete;

  constexpr const char* get_class() const noexcept {
    return "ComponentQuery";
  }

  constexpr int32_t get_hash() const noexcept {
    std::string_view name_view{C$ComponentQuery::get_class()};
    return static_cast<int32_t>(vk::murmur_hash<uint32_t>(name_view.data(), name_view.size()));
  }

  auto stream() noexcept -> kphp::component::stream& {
    return m_stream;
  }
};

// === component query client interface ===========================================================

inline auto f$component_client_send_request(string name, string request) noexcept -> kphp::coro::task<class_instance<C$ComponentQuery>> {
  auto expected_stream{kphp::component::stream::open({name.c_str(), name.size()}, k2::stream_kind::component)};
  if (!expected_stream) [[unlikely]] {
    co_return class_instance<C$ComponentQuery>{};
  }

  auto stream{std::move(*expected_stream)};
  auto request_span{std::span<const char>{request.c_str(), request.size()}};
  if (auto expected{co_await kphp::forks::id_managed(kphp::component::send_request(stream, std::as_bytes(request_span)))}; !expected) [[unlikely]] {
    co_return class_instance<C$ComponentQuery>{};
  }
  co_return make_instance<C$ComponentQuery>(std::move(stream));
}

inline auto f$component_client_fetch_response(class_instance<C$ComponentQuery> query) noexcept -> kphp::coro::task<string> {
  if (query.is_null()) [[unlikely]] {
    kphp::log::warning("can't fetch component response from null query");
    co_return string{};
  }

  string response{};
  if (auto expected{co_await kphp::forks::id_managed(kphp::component::fetch_response(query.get()->stream(), kphp::component::read_ext::append(response)))};
      !expected) [[unlikely]] {
    co_return string{};
  }
  co_return std::move(response);
}

// // === component query server interface ===========================================================

inline auto f$component_server_accept_query() noexcept -> kphp::coro::task<class_instance<C$ComponentQuery>> {
  auto opt_stream{co_await kphp::forks::id_managed(kphp::component::stream::accept())};
  if (!opt_stream) [[unlikely]] {
    co_return class_instance<C$ComponentQuery>{};
  }
  co_return make_instance<C$ComponentQuery>(std::move(*opt_stream));
}

inline auto f$component_server_fetch_request(class_instance<C$ComponentQuery> query) noexcept -> kphp::coro::task<string> {
  if (query.is_null()) [[unlikely]] {
    kphp::log::warning("can't fetch server request from null query");
    co_return string{};
  }

  string request{};
  if (auto expected{co_await kphp::forks::id_managed(kphp::component::fetch_request(query.get()->stream(), kphp::component::read_ext::append(request)))};
      !expected) [[unlikely]] {
    co_return string{};
  }
  co_return std::move(request);
}

inline auto f$component_server_send_response(class_instance<C$ComponentQuery> query, string response) noexcept -> kphp::coro::task<> {
  if (query.is_null()) [[unlikely]] {
    kphp::log::warning("can't send server response to null query");
    co_return;
  }

  auto& stream{query.get()->stream()};
  auto response_span{std::span<const char>{response.c_str(), response.size()}};
  co_await kphp::forks::id_managed(kphp::component::send_response(stream, std::as_bytes(response_span)));
}
