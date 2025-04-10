// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cmath>

#ifdef __x86_64__
#include <emmintrin.h>
#endif // __x86_64__

#ifdef __aarch64__
#include <arm_neon.h>
#endif // __arch64_-

static inline double __dot_product(const double* x, const double* y, const int size) {
#if defined(__x86_64__)
  __v2df as, bs, result;
  double temp[2];
  temp[0] = temp[1] = 0.0;
  result = _mm_loadu_pd(temp);
  int i = 0;
  for (i = 0; i + 1 < size; i += 2) {
    as = _mm_loadu_pd(x + i);
    bs = _mm_loadu_pd(y + i);
    result += as * bs;
  }
  _mm_storeu_pd(temp, result);
  temp[0] += temp[1] + (i < size ? x[size - 1] * y[size - 1] : 0.0);
  return std::fpclassify(temp[0]) == FP_SUBNORMAL ? 0.0 : temp[0];
#elif defined(__arch64__)
  float64x2_t xf, yf, result;
  double temp[2] = {0.0, 0.0};
  result = vld1q_f64(temp);

  int i;
  for (i = 0; i + 1 < size; i += 2) {
    xf = vld1q_f64(x + i);
    yf = vld1q_f64(y + i);

    result = vaddq_f64(result, vmulq_f64(xf, yf));
  }
  vst1q_f64(temp, result);
  temp[0] += temp[1] + (i < size ? x[size - 1] * y[size - 1] : 0.0);

  return std::fpclassify(temp[0]) == FP_SUBNORMAL ? 0.0 : temp[0];
#else // __arch64__
  double result = 0.0;

  for (int i = 0; i < size; ++i) {
    result += x[i] * y[i];
  }
  return std::fpclassify(result) == FP_SUBNORMAL ? 0.0 : result;
#endif
}
