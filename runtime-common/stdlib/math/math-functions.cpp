// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime-common/stdlib/math/math-functions.h"

#include <cstdlib>

mixed f$abs(const mixed &v) noexcept {
  mixed num = v.to_numeric();
  if (num.is_int()) {
    return std::abs(num.to_int());
  }
  return fabs(num.to_float());
}

int64_t f$abs(int64_t v) noexcept {
  return std::abs(v);
}

double f$abs(double v) noexcept {
  return std::abs(v);
}

int64_t f$abs(const Optional<int64_t> &v) noexcept {
  return f$abs(val(v));
}

int64_t f$abs(const Optional<bool> &v) noexcept {
  return f$abs(static_cast<int64_t>(val(v)));
}

double f$abs(const Optional<double> &v) noexcept {
  return f$abs(val(v));
}
