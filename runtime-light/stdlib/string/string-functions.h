// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cstdint>

#include "runtime-light/stdlib/string/string-context.h"

inline int64_t f$strlen(const string &s) noexcept {
  return s.size();
}

inline tmp_string f$_tmp_substr(const string &, int64_t, int64_t  = std::numeric_limits<int64_t>::max()) {
  php_critical_error("call to unsupported function");
}

inline tmp_string f$_tmp_substr(tmp_string , int64_t, int64_t  = std::numeric_limits<int64_t>::max()) {
  php_critical_error("call to unsupported function");
}

inline tmp_string f$_tmp_trim(tmp_string , const string & = string()) {
  php_critical_error("call to unsupported function");
}

inline tmp_string f$_tmp_trim(const string &, const string & = string()) {
  php_critical_error("call to unsupported function");
}

inline string f$trim(tmp_string , const string & = string()) {
  php_critical_error("call to unsupported function");
}

inline string f$trim(const string &, const string & = string()) {
  php_critical_error("call to unsupported function");
}

inline Optional<string> f$substr(const string &, int64_t, int64_t  = std::numeric_limits<int64_t>::max()) {
  php_critical_error("call to unsupported function");
}

inline Optional<string> f$substr(tmp_string, int64_t, int64_t  = std::numeric_limits<int64_t>::max()) {
  php_critical_error("call to unsupported function");
}


inline string f$pack(const string &, const array<mixed> &) {
  php_critical_error("call to unsupported function");
}

inline Optional<array<mixed>> f$unpack(const string &, const string &) {
  php_critical_error("call to unsupported function");
}

inline mixed f$str_replace(const mixed &, const mixed &, const mixed &, int64_t & = StringComponentContext::get().str_replace_count_dummy) {
  php_critical_error("call to unsupported function");
}

inline string f$str_replace(const string &, const string &, const string &, int64_t & = StringComponentContext::get().str_replace_count_dummy) {
  php_critical_error("call to unsupported function");
}

template<typename T1, typename T2>
string f$str_replace(const array<T1> &, const array<T2> &, const string &, int64_t & = StringComponentContext::get().str_replace_count_dummy) {
  php_critical_error("call to unsupported function");
}

inline string f$str_replace(const mixed &, const mixed &, const string &, int64_t & = StringComponentContext::get().str_replace_count_dummy) {
  php_critical_error("call to unsupported function");
}

template<class T1, class T2, class SubjectT, class = enable_if_t_is_optional_string<SubjectT>>
SubjectT f$str_replace(const T1 &search, const T2 &replace, const SubjectT &subject, int64_t &replace_count = StringComponentContext::get().str_replace_count_dummy) {
  return f$str_replace(search, replace, subject.val(), replace_count);
}

inline string f$str_ireplace(const string &, const string &, const string &, int64_t & = StringComponentContext::get().str_replace_count_dummy) {
  php_critical_error("call to unsupported function");
}

template<typename T1, typename T2>
string f$str_ireplace(const array<T1> &, const array<T2> &, const string &, int64_t & = StringComponentContext::get().str_replace_count_dummy) {
  php_critical_error("call to unsupported function");
}

inline string f$str_ireplace(const mixed &, const mixed &, const string &, int64_t & = StringComponentContext::get().str_replace_count_dummy) {
  php_critical_error("call to unsupported function");
}

template<class T1, class T2, class SubjectT, class = enable_if_t_is_optional_string<SubjectT>>
SubjectT f$str_ireplace(const T1 &search, const T2 &replace, const SubjectT &subject, int64_t &replace_count = StringComponentContext::get().str_replace_count_dummy) {
  return f$str_ireplace(search, replace, subject.val(), replace_count);
}

inline mixed f$str_ireplace(const mixed &, const mixed &, const mixed &, int64_t & = StringComponentContext::get().str_replace_count_dummy) {
  php_critical_error("call to unsupported function");
}

inline int64_t f$similar_text(const string &, const string &, double & = StringComponentContext::get().default_similar_text_percent_stub) {
  php_critical_error("call to unsupported function");
}