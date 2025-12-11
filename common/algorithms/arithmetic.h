// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

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
