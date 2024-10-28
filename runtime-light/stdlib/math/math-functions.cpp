// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime-light/stdlib/math/math-functions.h"

#include <stdlib.h>

mixed f$abs(const mixed &v) {
  mixed num = v.to_numeric();
  if (num.is_int()) {
    return std::abs(num.to_int());
  }
  return fabs(num.to_float());
}

int64_t f$abs(int64_t v) {
  return std::abs(v);
}

double f$abs(double v) {
  return std::abs(v);
}

int64_t f$abs(const Optional<int64_t> &v) {
  return f$abs(val(v));
}

int64_t f$abs(const Optional<bool> &v) {
  return f$abs(static_cast<int64_t>(val(v)));
}

double f$abs(const Optional<double> &v) {
  return f$abs(val(v));
}


