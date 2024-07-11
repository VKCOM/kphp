// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cstdint>

#include "runtime-core/runtime-core.h"

int64_t f$bindec(const string &number) noexcept;

string f$decbin(int64_t number) noexcept;

string f$dechex(int64_t number) noexcept;

int64_t f$hexdec(const string &number) noexcept;

double f$lcg_value();


void f$mt_srand(int64_t seed = std::numeric_limits<int64_t>::min()) noexcept;

int64_t f$mt_rand(int64_t l, int64_t r) noexcept;

int64_t f$mt_rand() noexcept;

int64_t f$mt_getrandmax() noexcept;


void f$srand(int64_t seed = std::numeric_limits<int64_t>::min()) noexcept;

int64_t f$rand(int64_t l, int64_t r) noexcept;

int64_t f$rand() noexcept;

int64_t f$getrandmax() noexcept;

Optional<int64_t> f$random_int(int64_t l, int64_t r) noexcept;

Optional<string> f$random_bytes(int64_t length) noexcept;


template<class T>
inline T f$min(const array<T> &a);

template<class T>
inline T f$max(const array<T> &a);

template<class T>
inline T f$min(const T &arg1);

template<class T, class... Args>
inline T f$min(const T &arg1, const T &arg2, Args&&... args);

template<class T>
inline T f$max(const T &arg1);

template<class T, class... Args>
inline T f$max(const T &arg1, const T &arg2, Args&&... args);

// TODO REMOVE?
constexpr int64_t PHP_ROUND_HALF_UP = 123423141;
constexpr int64_t PHP_ROUND_HALF_DOWN = 123423144;
constexpr int64_t PHP_ROUND_HALF_EVEN = 123423145;
constexpr int64_t PHP_ROUND_HALF_ODD = 123423146;

mixed f$abs(const mixed &v);

int64_t f$abs(int64_t v);

double f$abs(double v);

int64_t f$abs(const Optional<int64_t> &v);

int64_t f$abs(const Optional<bool> &v);

double f$abs(const Optional<double> &v);

inline double f$acos(double v);

inline double f$atan(double v);

inline double f$atan2(double y, double x);

string f$base_convert(const string &number, int64_t frombase, int64_t tobase);

inline double f$ceil(double v);

inline double f$cos(double v);

inline double f$cosh(double v);

inline double f$acosh(double v);

inline double f$deg2rad(double v);

inline double f$exp(double v);

inline double f$floor(double v);

inline double f$fmod(double x, double y);

inline bool f$is_finite(double v);

inline bool f$is_infinite(double v);

inline bool f$is_nan(double v);

inline double f$log(double v);

inline double f$log(double v, double base);

inline double f$pi();

double f$round(double v, int64_t precision = 0);

inline double f$sin(double v);

inline double f$sinh(double v);

inline double f$sqrt(double v);

inline double f$tan(double v);

inline double f$asin(double v);

inline double f$asinh(double v);

inline double f$rad2deg(double v);

void init_math_functions() noexcept;

/*
 *
 *     IMPLEMENTATION
 *
 */


template<class T>
T f$min(const array<T> &a) {
  if (a.count() == 0) {
    php_warning("Empty array specified to function min");
    return T();
  }

  typename array<T>::const_iterator p = a.begin();
  T res = p.get_value();
  for (++p; p != a.end(); ++p) {
    if (lt(p.get_value(), res)) {
      res = p.get_value();
    }
  }
  return res;
}

template<class T>
T f$max(const array<T> &a) {
  if (a.count() == 0) {
    php_warning("Empty array specified to function max");
    return T();
  }

  typename array<T>::const_iterator p = a.begin();
  T res = p.get_value();
  for (++p; p != a.end(); ++p) {
    if (lt(res, p.get_value())) {
      res = p.get_value();
    }
  }
  return res;
}

template<class T>
T f$min(const T &arg1) {
  return arg1;
}

template<class T, class ...Args>
T f$min(const T &arg1, const T &arg2, Args&& ...args) {
  return f$min<T>(lt(arg1, arg2) ? arg1 : arg2, std::forward<Args>(args)...);
}

template<class T>
T f$max(const T &arg1) {
  return arg1;
}

template<class T, class ...Args>
T f$max(const T &arg1, const T &arg2, Args&& ...args) {
  return f$max<T>(lt(arg2, arg1) ? arg1 : arg2, std::forward<Args>(args)...);
}

double f$acos(double v) {
  return acos(v);
}

double f$atan(double v) {
  return atan(v);
}

double f$atan2(double y, double x) {
  return atan2(y, x);
}

double f$ceil(double v) {
  return ceil(v);
}

double f$cos(double v) {
  return cos(v);
}

double f$cosh(double v) {
  return cosh(v);
}

double f$acosh(double v) {
  return acosh(v);
}

double f$deg2rad(double v) {
  return v * M_PI / 180;
}

double f$exp(double v) {
  return exp(v);
}

double f$floor(double v) {
  return floor(v);
}

double f$fmod(double x, double y) {
  if (fabs(x) > 1e100 || fabs(y) < 1e-100) {
    return 0.0;
  }
  return fmod(x, y);
}

bool f$is_finite(double v) {
  int v_class = std::fpclassify(v);
  return (v_class != FP_NAN && v_class != FP_INFINITE);
}

bool f$is_infinite(double v) {
  return (std::fpclassify(v) == FP_INFINITE);
}

bool f$is_nan(double v) {
  return (std::fpclassify(v) == FP_NAN);
}

double f$log(double v) {
  if (v <= 0.0) {
    return 0.0;
  }
  return log(v);
}

double f$log(double v, double base) {
  if (v <= 0.0 || base <= 0.0 || fabs(base - 1.0) < 1e-9) {
    return 0.0;
  }
  return log(v) / log(base);
}

double f$pi() {
  return M_PI;
}

double f$sin(double v) {
  return sin(v);
}

double f$sinh(double v) {
  return sinh(v);
}

double f$sqrt(double v) {
  if (v < 0) {
    return 0.0;
  }
  return sqrt(v);
}

double f$tan(double v) {
  return tan(v);
}

double f$asin(double v) {
  return asin(v);
}

double f$asinh(double v) {
  return asinh(v);
}

double f$rad2deg(double v) {
  return v * 180 / M_PI;
}
