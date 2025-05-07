// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cstdint>
#include <string_view>
#include <utility>

#include "runtime-common/core/runtime-core.h"
#include "runtime-common/stdlib/server/url-functions.h"
#include "runtime-light/server/http/http-server-state.h"
#include "runtime-light/utils/logs.h"

namespace kphp::http {

void header(std::string_view header, bool replace, int64_t response_code) noexcept;

} // namespace kphp::http

inline void f$header(const string& str, bool replace = true, int64_t response_code = kphp::http::status::NO_STATUS) noexcept {
  kphp::http::header({str.c_str(), str.size()}, replace, response_code);
}

void f$setrawcookie(const string& name, const string& value, int64_t expire_or_options = 0, const string& path = {}, const string& domain = {},
                    bool secure = false, bool http_only = false) noexcept;

inline void f$setcookie(const string& name, const string& value = {}, int64_t expire_or_options = 0, const string& path = {}, const string& domain = {},
                        bool secure = false, bool http_only = false) noexcept {
  f$setrawcookie(name, f$urlencode(value), expire_or_options, path, domain, secure, http_only);
}

inline array<string> f$headers_list() noexcept {
  const auto& headers{HttpServerInstanceState::get().headers()};
  constexpr std::string_view header_separator{": "};

  array<string> list{array_size{static_cast<int64_t>(headers.size()), true}};
  for (const auto& [header_name, header_value] : headers) {
    list.push_back(string{static_cast<string::size_type>(header_name.size() + header_value.size() + header_separator.size()), true}
                       .append(header_name.c_str())
                       .append(header_separator.data())
                       .append(header_value.c_str()));
  }

  return list;
}

inline bool f$headers_sent([[maybe_unused]] Optional<std::optional<std::reference_wrapper<string>>> filename = {},
                           [[maybe_unused]] Optional<std::optional<std::reference_wrapper<string>>> line = {}) noexcept {
  kphp::log::warning("called stub headers_sent");
  return false;
}

template<typename F>
bool f$header_register_callback(F&& /*unused*/) noexcept {
  kphp::log::warning("called stub header_register_callback");
  return true;
}

template<class T>
string f$http_build_query(const array<T>& /*a*/, const string& /*numeric_prefix*/ = {}, const string& /*arg_separator*/ = string(), int64_t /*enc_type*/ = 1) {
  kphp::log::fatal("call to unsupported function");
}
