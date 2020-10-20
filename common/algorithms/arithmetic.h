#ifndef ENGINE_ALGO_ARITHMETIC_H
#define ENGINE_ALGO_ARITHMETIC_H

#include <limits.h>
#include <stdlib.h>

static inline int saturating_abs(int x) {
  if (x == INT_MIN) {
    return INT_MAX;
  }
  return abs(x);
}

static inline int align4(int x) {
  return (x + 3) & -4;
}

#endif // ENGINE_ALGO_ARITHMETIC_H
