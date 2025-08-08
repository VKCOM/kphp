// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

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
#include "runtime-light/stdlib/diagnostics/diagnostics.h"
#include "runtime-light/stdlib/fork/fork-functions.h"
#include "runtime-light/streams/stream.h"

namespace kphp::component {

inline auto send_request(const kphp::component::stream& stream, std::span<const std::byte> request) noexcept -> kphp::coro::task<std::expected<void, int32_t>> {
  if (auto expected{co_await stream.write(request)}; !expected) [[unlikely]] {
    co_return std::move(expected);
  }
  stream.shutdown_write();
  co_return std::expected<void, int32_t>{};
}

inline auto fetch_response(kphp::component::stream& stream) noexcept -> kphp::coro::task<std::expected<void, int32_t>> {
  stream.clear();
  co_return co_await stream.read();
}

inline auto fetch_request(kphp::component::stream& stream) noexcept -> kphp::coro::task<std::expected<void, int32_t>> {
  stream.clear();
  co_return co_await stream.read();
}

inline auto send_response(const kphp::component::stream& stream,
                          std::span<const std::byte> response) noexcept -> kphp::coro::task<std::expected<void, int32_t>> {
  if (auto expected{co_await stream.write(response)}; !expected) [[unlikely]] {
    co_return std::move(expected);
  }
  stream.shutdown_write();
  co_return std::expected<void, int32_t>{};
}

inline auto query(kphp::component::stream& stream, std::span<const std::byte> request) noexcept -> kphp::coro::task<std::expected<void, int32_t>> {
  if (auto expected{co_await send_request(stream, request)}; !expected) [[unlikely]] {
    co_return std::move(expected);
  }
  if (auto expected{co_await fetch_response(stream)}; !expected) [[unlikely]] {
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

inline auto f$component_client_send_request(string name, string message) noexcept -> kphp::coro::task<class_instance<C$ComponentQuery>> {
  auto expected_stream{kphp::component::stream::open({name.c_str(), name.size()}, k2::stream_kind::component)};
  if (!expected_stream) [[unlikely]] {
    co_return class_instance<C$ComponentQuery>{};
  }

  auto stream{std::move(*expected_stream)};
  std::span<const std::byte> request{reinterpret_cast<const std::byte*>(message.c_str()), message.size()};
  if (auto expected{co_await kphp::forks::id_managed(kphp::component::send_request(stream, request))}; !expected) [[unlikely]] {
    co_return class_instance<C$ComponentQuery>{};
  }
  co_return make_instance<C$ComponentQuery>(std::move(stream));
}

inline auto f$component_client_fetch_response(class_instance<C$ComponentQuery> query) noexcept -> kphp::coro::task<string> {
  if (query.is_null()) [[unlikely]] {
    kphp::log::warning("can't fetch component response from null query");
    co_return string{};
  }

  auto& stream{query.get()->stream()};
  if (auto expected{co_await kphp::forks::id_managed(kphp::component::fetch_response(stream))}; !expected) [[unlikely]] {
    co_return string{};
  }
  co_return string{reinterpret_cast<const char*>(stream.data().data()), static_cast<string::size_type>(stream.data().size())};
}

// === component query server interface ===========================================================

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

  auto& stream{query.get()->stream()};
  if (auto expected{co_await kphp::forks::id_managed(kphp::component::fetch_request(stream))}; !expected) [[unlikely]] {
    co_return string{};
  }
  co_return string{reinterpret_cast<const char*>(stream.data().data()), static_cast<string::size_type>(stream.data().size())};
}

inline auto f$component_server_send_response(class_instance<C$ComponentQuery> query, string message) noexcept -> kphp::coro::task<> {
  if (query.is_null()) [[unlikely]] {
    kphp::log::warning("can't send server response to null query");
    co_return;
  }

  auto& stream{query.get()->stream()};
  std::span<const std::byte> response{reinterpret_cast<const std::byte*>(message.c_str()), message.size()};
  co_await kphp::forks::id_managed(kphp::component::send_response(stream, response));
}
