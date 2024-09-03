// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include <cstdint>

#include "runtime-core/runtime-core.h"

inline int64_t f$strlen(const string &s) noexcept {
  return s.size();
}

inline tmp_string f$_tmp_substr(const string &str, int64_t start, int64_t length = std::numeric_limits<int64_t>::max()) {
  php_critical_error("call to unsupported function");
}

inline tmp_string f$_tmp_substr(tmp_string str, int64_t start, int64_t length = std::numeric_limits<int64_t>::max()) {
  php_critical_error("call to unsupported function");
}

inline tmp_string f$_tmp_trim(tmp_string s, const string &what = string()) {
  php_critical_error("call to unsupported function");
}

inline tmp_string f$_tmp_trim(const string &s, const string &what = string()) {
  php_critical_error("call to unsupported function");
}

inline string f$trim(tmp_string s, const string &what = string()) {
  php_critical_error("call to unsupported function");
}

inline string f$trim(const string &s, const string &what = string()) {
  php_critical_error("call to unsupported function");
}

inline Optional<string> f$substr(const string &str, int64_t start, int64_t length = std::numeric_limits<int64_t>::max()) {
  php_critical_error("call to unsupported function");
}

inline Optional<string> f$substr(tmp_string, int64_t start, int64_t length = std::numeric_limits<int64_t>::max()) {
  php_critical_error("call to unsupported function");
}
