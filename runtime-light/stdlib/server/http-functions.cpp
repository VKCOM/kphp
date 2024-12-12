// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime-light/stdlib/server/http-functions.h"

#include <algorithm>
#include <cctype>
#include <cinttypes>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <ranges>
#include <string_view>
#include <utility>

#include "absl/strings/ascii.h"
#include "absl/strings/str_split.h"

#include "runtime-common/core/runtime-core.h"
#include "runtime-common/core/utils/kphp-assert-core.h"
#include "runtime-light/server/http/http-server-state.h"
#include "runtime-light/stdlib/time/time-functions.h"

namespace {

constexpr std::string_view HTTP_DATE = R"(D, d M Y H:i:s \G\M\T)";
constexpr std::string_view HTTP_STATUS_PREFIX = "http/";
constexpr std::string_view HTTP_LOCATION_HEADER_PREFIX = "location:";

bool http_status_header(std::string_view header) noexcept {
  const auto lowercase_prefix{header | std::views::take(HTTP_STATUS_PREFIX.size()) | std::views::transform([](auto c) noexcept { return std::tolower(c); })};
  return std::ranges::equal(lowercase_prefix, HTTP_STATUS_PREFIX);
}

bool http_location_header(std::string_view header) noexcept {
  const auto lowercase_prefix{header | std::views::take(HTTP_LOCATION_HEADER_PREFIX.size())
                              | std::views::transform([](auto c) noexcept { return std::tolower(c); })};
  return std::ranges::equal(lowercase_prefix, HTTP_LOCATION_HEADER_PREFIX);
}

std::optional<uint64_t> valid_http_status_header(std::string_view header) noexcept {
  const auto header_size{header.size()};
  uint64_t http_response_code{};

  if (header_size < HTTP_STATUS_PREFIX.size()) [[unlikely]] {
    return {};
  }
  // skip digits
  size_t pos{HTTP_STATUS_PREFIX.size()};
  for (; pos < header_size && std::isdigit(header[pos]); ++pos) {
  }
  // expect '.'
  if (pos == header_size || header[pos++] != '.') [[unlikely]] {
    return {};
  }
  // skip digits
  for (; pos < header_size && std::isdigit(header[pos]); ++pos) {
  }
  // expect ' '
  if (pos == header_size || header[pos++] != ' ') [[unlikely]] {
    return {};
  }
  // expect 3 digits http code
  if (pos == header_size || !std::isdigit(header[pos])) [[unlikely]] {
    return {};
  }
  http_response_code = header[pos++] - '0';
  if (pos == header_size || !std::isdigit(header[pos])) [[unlikely]] {
    return {};
  }
  http_response_code *= 10;
  http_response_code += header[pos++] - '0';
  if (pos == header_size || !std::isdigit(header[pos])) [[unlikely]] {
    return {};
  }
  http_response_code *= 10;
  http_response_code += header[pos++] - '0';
  // expect ' '
  if (pos == header_size || header[pos++] != ' ') [[unlikely]] {
    return {};
  }
  // expect all remaining characters to be printable
  for (; pos < header_size; ++pos) {
    if (std::isprint(header[pos]) == 0) [[unlikely]] {
      return {};
    }
  }
  return http_response_code;
}

} // namespace

void header(std::string_view header_view, bool replace, int64_t response_code) noexcept {
  if (response_code < 0) [[unlikely]] {
    return php_warning("http response code can't be negative: %" PRIi64, response_code);
  }

  auto &http_server_instance_st{HttpServerInstanceState::get()};
  // HTTP status special case
  if (http_status_header(header_view)) {
    if (const auto opt_status_code{valid_http_status_header(header_view)}; opt_status_code.has_value()) [[likely]] {
      http_server_instance_st.status_code = *opt_status_code;
      return;
    } else {
      php_error("invalid HTTP status header: %s", header_view.data());
    }
  }

  const std::pair<std::string_view, std::string_view> parts{absl::StrSplit(header_view, absl::MaxSplits(':', 1))};
  auto [name_view, value_view]{parts};
  if (name_view.size() + value_view.size() + 1 != header_view.size()) [[unlikely]] {
    return php_warning("invalid header: %s", header_view.data());
  }

  // validate header name
  name_view = absl::StripAsciiWhitespace(name_view);
  if (!std::ranges::all_of(name_view, [](char c) noexcept { return std::isalnum(c) || c == '-' || c == '_'; })) [[unlikely]] {
    return php_warning("invalid header name: %s", name_view.data());
  }
  // validate header value
  value_view = absl::StripAsciiWhitespace(value_view);
  if (!std::ranges::all_of(value_view, [](char c) noexcept { return std::isprint(c); })) [[unlikely]] {
    return php_warning("invalid header value: %s", value_view.data());
  }

  http_server_instance_st.add_header(name_view, value_view, replace);

  // Location: special case
  const bool can_return_redirect{
    response_code == HttpStatus::NO_STATUS && http_server_instance_st.status_code != HttpStatus::CREATED
    && (http_server_instance_st.status_code < HttpStatus::MULTIPLE_CHOICES || http_server_instance_st.status_code >= HttpStatus::BAD_REQUEST)};
  if (can_return_redirect && http_location_header(header_view)) {
    http_server_instance_st.status_code = HttpStatus::FOUND;
  }

  if (!header_view.empty() && response_code != HttpStatus::NO_STATUS) {
    http_server_instance_st.status_code = response_code;
  }
}

void f$setrawcookie(const string &name, const string &value, int64_t expire_or_options, const string &path, const string &domain, bool secure,
                    bool http_only) noexcept {
  auto &static_SB_spare{RuntimeContext::get().static_SB_spare};

  static_SB_spare.clean() << HttpHeader::SET_COOKIE.data() << ": " << name << '=';
  if (value.empty()) {
    static_SB_spare << "DELETED; expires=Thu, 01 Jan 1970 00:00:01 GMT";
  } else {
    static_SB_spare << value;
    if (expire_or_options != 0) {
      static_SB_spare << "; expires=" << f$gmdate({HTTP_DATE.data(), HTTP_DATE.size()}, expire_or_options);
    }
  }
  if (!path.empty()) {
    static_SB_spare << "; path=" << path;
  }
  if (!domain.empty()) {
    static_SB_spare << "; domain=" << domain;
  }
  if (secure) {
    static_SB_spare << "; secure";
  }
  if (http_only) {
    static_SB_spare << "; HttpOnly";
  }
  header({static_SB_spare.c_str(), static_SB_spare.size()}, false, HttpStatus::NO_STATUS);
}
