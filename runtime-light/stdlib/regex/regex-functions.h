// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "runtime-core/runtime-core.h"

struct RegexComponentState {
  int64_t preg_replace_count_dummy{};

  static RegexComponentState &get() noexcept;
};

class regexp : vk::not_copyable {
public:
  regexp() = default;

  explicit regexp(const string &) {}
  regexp(const char *, int64_t) {}

  void init(const string &, const char * = nullptr, const char * = nullptr) {}
  void init(const char *, int64_t, const char * = nullptr, const char * = nullptr) {}
};

template<class T1, class T2, class T3, class = enable_if_t_is_optional<T3>>
auto f$preg_replace(const T1 &regex, const T2 &replace_val, const T3 &subject, int64_t limit = -1,
                    int64_t &replace_count = RegexComponentState::get().preg_replace_count_dummy) {
  return f$preg_replace(regex, replace_val, subject.val(), limit, replace_count);
}

inline Optional<string> f$preg_replace(const regexp &, const string &, const string &, int64_t = -1,
                                       int64_t & = RegexComponentState::get().preg_replace_count_dummy) {
  php_critical_error("call to unsupported function");
}

inline Optional<string> f$preg_replace(const regexp &, const mixed &, const string &, int64_t = -1,
                                       int64_t & = RegexComponentState::get().preg_replace_count_dummy) {
  php_critical_error("call to unsupported function");
}

inline mixed f$preg_replace(const regexp &, const string &, const mixed &, int64_t = -1, int64_t & = RegexComponentState::get().preg_replace_count_dummy) {
  php_critical_error("call to unsupported function");
}

inline mixed f$preg_replace(const regexp &, const mixed &, const mixed &, int64_t = -1, int64_t & = RegexComponentState::get().preg_replace_count_dummy) {
  php_critical_error("call to unsupported function");
}

template<class T1, class T2>
auto f$preg_replace(const string &regex, const T1 &replace_val, const T2 &subject, int64_t limit,
                    int64_t &replace_count = RegexComponentState::get().preg_replace_count_dummy) {
  return f$preg_replace(regexp(regex), replace_val, subject, limit, replace_count);
}

inline Optional<string> f$preg_replace(const mixed &, const string &, const string &, int64_t = -1,
                                       int64_t & = RegexComponentState::get().preg_replace_count_dummy) {
  php_critical_error("call to unsupported function");
}

inline mixed f$preg_replace(const mixed &, const string &, const mixed &, int64_t = -1, int64_t & = RegexComponentState::get().preg_replace_count_dummy) {
  php_critical_error("call to unsupported function");
}

inline Optional<string> f$preg_replace(const mixed &, const mixed &, const string &, int64_t = -1,
                                       int64_t & = RegexComponentState::get().preg_replace_count_dummy) {
  php_critical_error("call to unsupported function");
}

inline mixed f$preg_replace(const mixed &, const mixed &, const mixed &, int64_t = -1, int64_t & = RegexComponentState::get().preg_replace_count_dummy) {
  php_critical_error("call to unsupported function");
}

template<class T1, class T2, class T3, class = enable_if_t_is_optional<T3>>
auto f$preg_replace_callback(const T1 &regex, const T2 &replace_val, const T3 &subject, int64_t limit = -1,
                             int64_t &replace_count = RegexComponentState::get().preg_replace_count_dummy) {
  return f$preg_replace_callback(regex, replace_val, subject.val(), limit, replace_count);
}

template<class T>
Optional<string> f$preg_replace_callback(const regexp &, const T &, const string &, int64_t = -1,
                                         int64_t & = RegexComponentState::get().preg_replace_count_dummy) {
  php_critical_error("call to unsupported function");
}

template<class T>
mixed f$preg_replace_callback(const regexp &, const T &, const mixed &, int64_t = -1, int64_t & = RegexComponentState::get().preg_replace_count_dummy) {
  php_critical_error("call to unsupported function");
}

template<class T, class T2>
auto f$preg_replace_callback(const string &regex, const T &replace_val, const T2 &subject, int64_t limit = -1,
                             int64_t &replace_count = RegexComponentState::get().preg_replace_count_dummy) {
  return f$preg_replace_callback(regexp(regex), replace_val, subject, limit, replace_count);
}

template<class T>
Optional<string> f$preg_replace_callback(const mixed &, const T &, const string &, int64_t = -1,
                                         int64_t & = RegexComponentState::get().preg_replace_count_dummy) {
  php_critical_error("call to unsupported function");
}

template<class T>
mixed f$preg_replace_callback(const mixed &, const T &, const mixed &, int64_t = -1, int64_t & = RegexComponentState::get().preg_replace_count_dummy) {
  php_critical_error("call to unsupported function");
}
