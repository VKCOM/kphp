// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <algorithm>
#include <cstdint>
#include <optional>
#include <string_view>

#include "common/mixin/not_copyable.h"
#include "runtime-common/core/runtime-core.h"
#include "runtime-light/allocator/allocator.h"
#include "runtime-light/core/std/containers.h"

namespace kphp {

namespace http {

enum class method : uint8_t { get, post, head, other };

enum class connection_kind : uint8_t { keep_alive, close };

enum status : uint16_t {
  no_status = 0,
  ok = 200,
  created = 201,
  multiple_choices = 300,
  found = 302,
  bad_request = 400,
};

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

} // namespace http

} // namespace kphp

struct HttpServerInstanceState final : private vk::not_copyable {
  using header_t = kphp::stl::string<kphp::memory::script_allocator>;
  using headers_map_t = kphp::stl::multimap<header_t, header_t, kphp::memory::script_allocator>;

  static constexpr auto ENCODING_GZIP = static_cast<uint32_t>(1U << 0U);
  static constexpr auto ENCODING_DEFLATE = static_cast<uint32_t>(1U << 1U);

  std::optional<string> opt_raw_post_data;

  uint32_t encoding{};
  uint64_t status_code{kphp::http::status::no_status};
  kphp::http::method http_method{kphp::http::method::other};
  kphp::http::connection_kind connection_kind{kphp::http::connection_kind::close};

private:
  headers_map_t headers_;

public:
  HttpServerInstanceState() noexcept = default;

  const headers_map_t &headers() const noexcept {
    return headers_;
  }

  void add_header(std::string_view name_view, std::string_view value_view, bool replace) noexcept {
    header_t name{name_view};
    std::ranges::for_each(name, [](auto &c) noexcept { c = std::tolower(c); });
    if (const auto it{headers_.find(name)}; replace && it != headers_.end()) [[unlikely]] {
      headers_.erase(name);
    }
    headers_.emplace(std::move(name), header_t{value_view});
  }

  static HttpServerInstanceState &get() noexcept;
};
