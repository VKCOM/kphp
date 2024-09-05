// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cstdint>

#include "runtime-core/runtime-core.h"

struct StringComponentState {
  int64_t str_replace_count_dummy{};
  double default_similar_text_percent_stub{};

  static StringComponentState &get() noexcept;
};

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


inline string f$pack(const string &pattern, const array<mixed> &a) {
  php_critical_error("call to unsupported function");
}

inline Optional<array<mixed>> f$unpack(const string &pattern, const string &data) {
  php_critical_error("call to unsupported function");
}

inline mixed f$str_replace(const mixed &search, const mixed &replace, const mixed &subject, int64_t &replace_count = StringComponentState::get().str_replace_count_dummy) {
  php_critical_error("call to unsupported function");
}

inline string f$str_replace(const string &search, const string &replace, const string &subject, int64_t &replace_count = StringComponentState::get().str_replace_count_dummy) {
  php_critical_error("call to unsupported function");
}

template<typename T1, typename T2>
string f$str_replace(const array<T1> &search, const array<T2> &replace, const string &subject, int64_t &replace_count = StringComponentState::get().str_replace_count_dummy) {
  php_critical_error("call to unsupported function");
}

inline string f$str_replace(const mixed &search, const mixed &replace, const string &subject, int64_t &replace_count = StringComponentState::get().str_replace_count_dummy) {
  php_critical_error("call to unsupported function");
}

template<class T1, class T2, class SubjectT, class = enable_if_t_is_optional_string<SubjectT>>
SubjectT f$str_replace(const T1 &search, const T2 &replace, const SubjectT &subject, int64_t &replace_count = StringComponentState::get().str_replace_count_dummy) {
  return f$str_replace(search, replace, subject.val(), replace_count);
}

inline string f$str_ireplace(const string &search, const string &replace, const string &subject, int64_t &replace_count = StringComponentState::get().str_replace_count_dummy) {
  php_critical_error("call to unsupported function");
}

template<typename T1, typename T2>
string f$str_ireplace(const array<T1> &search, const array<T2> &replace, const string &subject, int64_t &replace_count = StringComponentState::get().str_replace_count_dummy) {
  php_critical_error("call to unsupported function");
}

inline string f$str_ireplace(const mixed &search, const mixed &replace, const string &subject, int64_t &replace_count = StringComponentState::get().str_replace_count_dummy) {
  php_critical_error("call to unsupported function");
}

template<class T1, class T2, class SubjectT, class = enable_if_t_is_optional_string<SubjectT>>
SubjectT f$str_ireplace(const T1 &search, const T2 &replace, const SubjectT &subject, int64_t &replace_count = StringComponentState::get().str_replace_count_dummy) {
  return f$str_ireplace(search, replace, subject.val(), replace_count);
}

inline mixed f$str_ireplace(const mixed &search, const mixed &replace, const mixed &subject, int64_t &replace_count = StringComponentState::get().str_replace_count_dummy) {
  php_critical_error("call to unsupported function");
}

inline int64_t f$similar_text(const string &first, const string &second, double &percent = StringComponentState::get().default_similar_text_percent_stub) {
  php_critical_error("call to unsupported function");
}
