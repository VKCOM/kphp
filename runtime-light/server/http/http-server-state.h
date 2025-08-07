// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <algorithm>
#include <cstdint>
#include <locale>
#include <optional>
#include <string_view>

#include "common/mixin/not_copyable.h"
#include "runtime-common/core/allocator/script-allocator.h"
#include "runtime-common/core/runtime-core.h"
#include "runtime-common/core/std/containers.h"
#include "runtime-light/coroutine/task.h"
#include "runtime-light/streams/stream.h"

namespace kphp::http {

enum class method : uint8_t { get, post, head, other };

enum class connection_kind : uint8_t { keep_alive, close };

enum status : uint16_t {
  NO_STATUS = 0,
  OK = 200,
  CREATED = 201,
  MULTIPLE_CHOICES = 300,
  FOUND = 302,
  BAD_REQUEST = 400,
};

enum class response_state : uint8_t { not_started, sending_headers, headers_sent, sending_body, completed };

namespace headers {

inline constexpr std::string_view HOST = "host";
inline constexpr std::string_view COOKIE = "cookie";
inline constexpr std::string_view LOCATION = "location";
inline constexpr std::string_view SET_COOKIE = "set-cookie";
inline constexpr std::string_view CONNECTION = "connection";
inline constexpr std::string_view CONTENT_TYPE = "content-type";
inline constexpr std::string_view CONTENT_LENGTH = "content-length";
inline constexpr std::string_view AUTHORIZATION = "authorization";
inline constexpr std::string_view ACCEPT_ENCODING = "accept-encoding";
inline constexpr std::string_view CONTENT_ENCODING = "content-encoding";

} // namespace headers

} // namespace kphp::http

struct HttpServerInstanceState final : private vk::not_copyable {
  static constexpr auto ENCODING_GZIP = static_cast<uint32_t>(1U << 0U);
  static constexpr auto ENCODING_DEFLATE = static_cast<uint32_t>(1U << 1U);

  std::optional<kphp::component::stream> request_stream;

  std::optional<string> opt_raw_post_data;

  uint32_t encoding{};
  uint64_t status_code{kphp::http::status::NO_STATUS};
  kphp::http::method http_method{kphp::http::method::other};
  kphp::http::connection_kind connection_kind{kphp::http::connection_kind::close};
  kphp::http::response_state response_state{kphp::http::response_state::not_started};

  // The headers_registered_callback function should only be invoked once
  std::optional<kphp::coro::task<>> headers_registered_callback;

private:
  kphp::stl::multimap<kphp::stl::string<kphp::memory::script_allocator>, kphp::stl::string<kphp::memory::script_allocator>, kphp::memory::script_allocator>
      headers_;

public:
  HttpServerInstanceState() noexcept = default;

  const auto& headers() const noexcept {
    return headers_;
  }

  void add_header(std::string_view name_view, std::string_view value_view, bool replace) noexcept {
    kphp::stl::string<kphp::memory::script_allocator> name{name_view};
    std::ranges::for_each(name, [](auto& c) noexcept { c = std::tolower(c, std::locale::classic()); });
    if (const auto it{headers_.find(name)}; replace && it != headers_.end()) [[unlikely]] {
      headers_.erase(name);
    }
    headers_.emplace(std::move(name), kphp::stl::string<kphp::memory::script_allocator>{value_view});
  }

  static HttpServerInstanceState& get() noexcept;
};
