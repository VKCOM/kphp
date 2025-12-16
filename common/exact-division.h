// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <sys/cdefs.h>

#include <assert.h>
#include <stdint.h>

// Hacker's Delight Second Edition
// 10-16 Exact Division by Constants

struct exact_division {
  uint32_t shift;
  uint32_t mulinv;
};
typedef struct exact_division exact_division_t;

static inline void exact_division_init(exact_division_t* exact_division, uint32_t divisor) {
  assert(divisor);

  exact_division->shift = __builtin_ctz(divisor);

  divisor = divisor >> exact_division->shift;

  if (!divisor) {
    exact_division->mulinv = 1;
    return;
  }

  exact_division->mulinv = divisor;
  for (;;) {
    const uint32_t t = divisor * exact_division->mulinv;
    if (t == 1) {
      return;
    }

    exact_division->mulinv *= 2 - t;
  }
}

static inline uint32_t exact_division(exact_division_t* exact_division, uint64_t dividend) {
  uint64_t quotient = dividend >> exact_division->shift;
  quotient *= exact_division->mulinv;

  return (uint32_t)quotient;
}
