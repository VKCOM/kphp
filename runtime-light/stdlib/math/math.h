// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "runtime-common/core/runtime-core.h"

inline mixed f$abs(const mixed &v) {
  php_critical_error("call to unsupported function");
}

inline int64_t f$abs(int64_t v) {
  php_critical_error("call to unsupported function");
}

inline double f$abs(double v) {
  php_critical_error("call to unsupported function");
}

inline int64_t f$abs(const Optional<int64_t> &v) {
  php_critical_error("call to unsupported function");
}

inline int64_t f$abs(const Optional<bool> &v) {
  php_critical_error("call to unsupported function");
}

inline double f$abs(const Optional<double> &v) {
  php_critical_error("call to unsupported function");
}

template<class T>
T f$min(const array<T> &a) {
  php_critical_error("call to unsupported function");
}

template<class T>
T f$max(const array<T> &a) {
  php_critical_error("call to unsupported function");
}

template<class T>
T f$min(const T &arg1) {
  php_critical_error("call to unsupported function");
}

template<class T, class... Args>
T f$min(const T &arg1, const T &arg2, Args&&... args) {
  php_critical_error("call to unsupported function");
}

template<class T>
T f$max(const T &arg1) {
  php_critical_error("call to unsupported function");
}

template<class T, class... Args>
T f$max(const T &arg1, const T &arg2, Args&&... args) {
  php_critical_error("call to unsupported function");
}
