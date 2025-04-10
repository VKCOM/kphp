// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "runtime-common/core/runtime-core.h"

#include "runtime-common/stdlib/server/url-functions.h"
#include "runtime/context/runtime-context.h"

constexpr int64_t PHP_QUERY_RFC1738 = 1;
constexpr int64_t PHP_QUERY_RFC3986 = 2;
extern string AMPERSAND;

template<class T>
string f$http_build_query(const array<T>& a, const string& numeric_prefix = {}, const string& arg_separator = AMPERSAND, int64_t enc_type = PHP_QUERY_RFC1738);

/*
 *
 *     IMPLEMENTATION
 *
 */

template<class T>
string http_build_query_get_param_array(const string& key, const array<T>& a, const string& arg_separator, int64_t enc_type) {
  string result;
  bool first = true;
  for (typename array<T>::const_iterator p = a.begin(); p != a.end(); ++p) {
    const string& key_value_param =
        http_build_query_get_param((kphp_runtime_context.static_SB.clean() << key << '[' << p.get_key() << ']').str(), p.get_value(), arg_separator, enc_type);
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
string http_build_query_get_param(const string& key, const T& a, const string& arg_separator, int64_t enc_type) {
  if (f$is_null(a)) {
    return {};
  }
  if (f$is_array(a)) {
    return http_build_query_get_param_array(key, f$arrayval(a), arg_separator, enc_type);
  } else {
    auto encode = [&](string s) {
      if (enc_type == PHP_QUERY_RFC1738) {
        return f$urlencode(s);
      } else {
        return f$rawurlencode(s);
      }
    };
    string key_encoded = encode(key);
    string value_encoded = encode(f$strval(a));
    return (kphp_runtime_context.static_SB.clean() << key_encoded << '=' << value_encoded).str();
  }
}

template<class T>
string f$http_build_query(const array<T>& a, const string& numeric_prefix, const string& arg_separator, int64_t enc_type) {
  if (!vk::any_of_equal(enc_type, PHP_QUERY_RFC1738, PHP_QUERY_RFC3986)) {
    php_warning("Unknown enc type %ld in http_build_query", enc_type);
    enc_type = PHP_QUERY_RFC1738;
  }
  string result;
  bool first = true;
  for (typename array<T>::const_iterator p = a.begin(); p != a.end(); ++p) {
    string key;
    if (p.get_key().is_int()) {
      key = numeric_prefix;
      key.append(p.get_key());
    } else {
      key = f$strval(p.get_key());
    }
    string param = http_build_query_get_param(key, p.get_value(), arg_separator, enc_type);
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
