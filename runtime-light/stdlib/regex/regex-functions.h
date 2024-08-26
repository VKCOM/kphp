// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt


#pragma once

#include "runtime-core/runtime-core.h"

struct RegexComponentState {
  int64_t preg_replace_count_dummy{};

  static RegexComponentState& get() noexcept;
};


template<class T1, class T2, class T3, class = enable_if_t_is_optional<T3>>
auto f$preg_replace(const T1 &, const T2 &, const T3 &, int64_t  = -1, int64_t & = RegexComponentState::get().preg_replace_count_dummy) {
  php_critical_error("call to unsupported function");
}

template<class T1, class T2, class T3, class = enable_if_t_is_optional<T3>>
auto f$preg_replace_callback(const T1 &, const T2 &, const T3 &, int64_t  = -1, int64_t & = RegexComponentState::get().preg_replace_count_dummy) {
  php_critical_error("call to unsupported function");
}

template<class Regexp, class T>
Optional<string> f$preg_replace_callback(const Regexp &, const T &, const string &, int64_t  = -1, int64_t & = RegexComponentState::get().preg_replace_count_dummy) {
  php_critical_error("call to unsupported function");
}

template<class Regexp, class T>
mixed f$preg_replace_callback(const Regexp &, const T &, const mixed &, int64_t  = -1, int64_t & = RegexComponentState::get().preg_replace_count_dummy) {
  php_critical_error("call to unsupported function");
}

template<class T, class T2>
auto f$preg_replace_callback(const string &, const T &, const T2 &, int64_t  = -1, int64_t & = RegexComponentState::get().preg_replace_count_dummy) {
  php_critical_error("call to unsupported function");
}

template<class T>
Optional<string> f$preg_replace_callback(const mixed &, const T &, const string &, int64_t  = -1, int64_t & = RegexComponentState::get().preg_replace_count_dummy) {
  php_critical_error("call to unsupported function");
}

template<class T>
mixed f$preg_replace_callback(const mixed &, const T &, const mixed &, int64_t  = -1, int64_t & = RegexComponentState::get().preg_replace_count_dummy) {
  php_critical_error("call to unsupported function");
}
