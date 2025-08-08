// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "runtime-common/core/runtime-core.h"
#include "runtime-common/stdlib/math/math-functions.h"

int64_t f$bindec(const string& number) noexcept;

string f$decbin(int64_t number) noexcept;

double f$lcg_value();

void f$mt_srand(int64_t seed = std::numeric_limits<int64_t>::min()) noexcept;

int64_t f$mt_rand(int64_t l, int64_t r) noexcept;

int64_t f$mt_rand() noexcept;

void f$srand(int64_t seed = std::numeric_limits<int64_t>::min()) noexcept;

int64_t f$rand(int64_t l, int64_t r) noexcept;

int64_t f$rand() noexcept;

Optional<int64_t> f$random_int(int64_t l, int64_t r) noexcept;

Optional<string> f$random_bytes(int64_t length) noexcept;

// TODO REMOVE?
constexpr int64_t PHP_ROUND_HALF_UP = 123423141;
constexpr int64_t PHP_ROUND_HALF_DOWN = 123423144;
constexpr int64_t PHP_ROUND_HALF_EVEN = 123423145;
constexpr int64_t PHP_ROUND_HALF_ODD = 123423146;

inline double f$acos(double v);

inline double f$atan(double v);

inline double f$atan2(double y, double x);

inline double f$cosh(double v);

inline double f$acosh(double v);

inline double f$exp(double v);

inline double f$fmod(double x, double y);

inline bool f$is_finite(double v);

inline bool f$is_infinite(double v);

inline double f$sin(double v);

inline double f$sinh(double v);

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

double f$acos(double v) {
  return acos(v);
}

double f$atan(double v) {
  return atan(v);
}

double f$atan2(double y, double x) {
  return atan2(y, x);
}

double f$cosh(double v) {
  return cosh(v);
}

double f$acosh(double v) {
  return acosh(v);
}

double f$exp(double v) {
  return exp(v);
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

double f$sin(double v) {
  return sin(v);
}

double f$sinh(double v) {
  return sinh(v);
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
