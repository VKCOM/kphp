// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime/url.h"

#include "common/macos-ports.h"
#include "runtime-common/stdlib/server/url-functions.h"
#include "runtime-common/stdlib/string/string-functions.h"
#include "runtime/array_functions.h"

string AMPERSAND("&", 1);


//string f$base64_decode (const string &s) {
//  int result_len = (s.size() + 3) / 4 * 3;
//  string res (result_len, false);
//  result_len = base64_decode (s.c_str(), reinterpret_cast <unsigned char *> (res.buffer()), result_len + 1);
//
//  if (result_len < 0) {
//    return string();
//  }
//
//  res.shrink (result_len);
//  return res;
//}

static void parse_str_set_array_value(mixed &arr, const char *left_br_pos, int key_len, const string &value) {
  php_assert (*left_br_pos == '[');
  const char *right_br_pos = (const char *)memchr(left_br_pos + 1, ']', key_len - 1);
  if (right_br_pos != nullptr) {
    string next_key(left_br_pos + 1, static_cast<string::size_type>(right_br_pos - left_br_pos - 1));
    if (!arr.is_array()) {
      arr = array<mixed>();
    }

    if (next_key.empty()) {
      next_key = string(arr.to_array().get_next_key());
    }

    if (right_br_pos[1] == '[') {
      parse_str_set_array_value(arr[next_key], right_br_pos + 1, (int)(left_br_pos + key_len - (right_br_pos + 1)), value);
    } else {
      arr.set_value(next_key, value);
    }
  } else {
    arr = value;
  }
}

void parse_str_set_value(mixed &arr, const string &key, const string &value) {
  const char *key_c = key.c_str();
  const char *left_br_pos = (const char *)memchr(key_c, '[', key.size());
  if (left_br_pos != nullptr) {
    return parse_str_set_array_value(arr[string(key_c, static_cast<string::size_type>(left_br_pos - key_c))], left_br_pos, (int)(key_c + key.size() - left_br_pos), value);
  }

  arr.set_value(key, value);
}

void f$parse_str(const string &str, mixed &arr) {
  if (f$trim(str).empty()) {
    arr = array<mixed>();
    return;
  }

  arr = array<mixed>();

  array<string> par = explode('&', str, INT_MAX);
  int64_t len = par.count();
  for (int64_t i = 0; i < len; i++) {
    const char *cur = par.get_value(i).c_str();
    const char *eq_pos = strchrnul(cur, '=');
    string value;
    if (*eq_pos) {
      value = f$urldecode(string(eq_pos + 1));
    }

    string key(cur, static_cast<string::size_type>(eq_pos - cur));
    key = f$urldecode(key);
    parse_str_set_value(arr, key, value);
  }
}
