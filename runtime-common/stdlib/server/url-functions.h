// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cstdint>

#include "common/algorithms/find.h"
#include "runtime-common/core/runtime-core.h"
#include "runtime-common/core/utils/kphp-assert-core.h"

namespace kphp::http {

inline constexpr int64_t PHP_QUERY_RFC1738 = 1;
inline constexpr int64_t PHP_QUERY_RFC3986 = 2;

} // namespace kphp::http

string f$base64_encode(const string& s) noexcept;

Optional<string> f$base64_decode(const string& s, bool strict = false) noexcept;

string f$rawurlencode(const string& s) noexcept;

string f$rawurldecode(const string& s) noexcept;

string f$urlencode(const string& s) noexcept;

string f$urldecode(const string& s) noexcept;

mixed f$parse_url(const string& s, int64_t component = -1) noexcept;

void parse_str_set_value(mixed& arr, const string& key, const string& value) noexcept;

void f$parse_str(const string& str, mixed& arr) noexcept;

namespace kphp::http::impl {

template<class T>
string build_query_get_param(const string& key, const T& param, const string& arg_separator, int64_t encoding_type) noexcept;

template<class T>
string build_query_get_param_array(const string& key, const array<T>& data, const string& arg_separator, int64_t encoding_type) noexcept {
  string result{};
  bool first{true};
  for (typename array<T>::const_iterator p{data.begin()}; p != data.end(); ++p) {
    string key_value_param{kphp::http::impl::build_query_get_param((RuntimeContext::get().static_SB.clean() << key << '[' << p.get_key() << ']').str(),
                                                                   p.get_value(), arg_separator, encoding_type)};
    if (!key_value_param.empty()) {
      if (!first) {
        result.append(arg_separator);
      }
      result.append(key_value_param);
      first = false;
    }
  }
  return result;
}

template<class T>
string build_query_get_param(const string& key, const T& param, const string& arg_separator, int64_t encoding_type) noexcept {
  if (f$is_null(param)) {
    return {};
  }
  if (f$is_array(param)) {
    return kphp::http::impl::build_query_get_param_array(key, f$arrayval(param), arg_separator, encoding_type);
  }

  const auto encode{[](const string& s, int64_t encoding_type) noexcept {
    if (encoding_type == kphp::http::PHP_QUERY_RFC1738) {
      return f$urlencode(s);
    } else {
      return f$rawurlencode(s);
    }
  }};
  string key_encoded{encode(key, encoding_type)};
  string value_encoded{encode(f$strval(param), encoding_type)};
  return (RuntimeContext::get().static_SB.clean() << key_encoded << '=' << value_encoded).str();
}

} // namespace kphp::http::impl

template<class T>
string f$http_build_query(const array<T>& data, const string& numeric_prefix = {}, const string& arg_separator = string{1, '&'},
                          int64_t encoding_type = kphp::http::PHP_QUERY_RFC1738) noexcept {
  if (!vk::any_of_equal(encoding_type, kphp::http::PHP_QUERY_RFC1738, kphp::http::PHP_QUERY_RFC3986)) {
    php_warning("unexpected encoding type %ld in http_build_query, RFC1738 will be used instead", encoding_type);
    encoding_type = kphp::http::PHP_QUERY_RFC1738;
  }

  string result{};
  bool first{true};
  for (typename array<T>::const_iterator p{data.cbegin()}; p != data.cend(); ++p) {
    string key{};
    if (p.get_key().is_int()) {
      key = numeric_prefix;
      key.append(p.get_key());
    } else {
      key = f$strval(p.get_key());
    }

    string param{kphp::http::impl::build_query_get_param(key, p.get_value(), arg_separator, encoding_type)};
    if (param.size()) {
      if (!first) {
        result.append(arg_separator);
      }
      first = false;

      result.append(param);
    }
  }

  return result;
}
