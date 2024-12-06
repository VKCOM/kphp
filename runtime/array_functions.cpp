// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime/array_functions.h"
#include "runtime-common/stdlib/string/string-functions.h"

array<mixed> range_int(int64_t from, int64_t to, int64_t step) {
  if (from < to) {
    if (step <= 0) {
      php_warning("Wrong parameters from = %" PRIi64 ", to = %" PRIi64 ", step = %" PRIi64 " in function range", from, to, step);
      return {};
    }
    array<mixed> res(array_size((to - from + step) / step, true));
    for (int64_t i = from; i <= to; i += step) {
      res.push_back(i);
    }
    return res;
  } else {
    if (step == 0) {
      php_warning("Wrong parameters from = %" PRIi64 ", to = %" PRIi64 ", step = %" PRIi64 " in function range", from, to, step);
      return {};
    }
    if (step < 0) {
      step = -step;
    }
    array<mixed> res(array_size((from - to + step) / step, true));
    for (int64_t i = from; i >= to; i -= step) {
      res.push_back(i);
    }
    return res;
  }
}

array<mixed> range_string(const string &from_s, const string &to_s, int64_t step) {
  if (from_s.empty() || to_s.empty() || from_s.size() > 1 || to_s.size() > 1) {
    php_warning("Wrong parameters \"%s\" and \"%s\" for function range", from_s.c_str(), to_s.c_str());
    return {};
  }
  if (step != 1) {
    php_critical_error ("unsupported step = %" PRIi64 " in function range", step);
  }
  const int64_t from = static_cast<unsigned char>(from_s[0]);
  const int64_t to = static_cast<unsigned char>(to_s[0]);
  if (from < to) {
    array<mixed> res(array_size(to - from + 1, true));
    for (int64_t i = from; i <= to; i++) {
      res.push_back(f$chr(i));
    }
    return res;
  } else {
    array<mixed> res(array_size(from - to + 1, true));
    for (int64_t i = from; i >= to; i--) {
      res.push_back(f$chr(i));
    }
    return res;
  }
}

array<mixed> f$range(const mixed &from, const mixed &to, int64_t step) {
  if ((from.is_string() && !from.is_numeric()) || (to.is_string() && !to.is_numeric())) {
    return range_string(from.to_string(), to.to_string(), step);
  }
  return range_int(from.to_int(), to.to_int(), step);
}

static_assert(sizeof(array<Unknown>) == SIZEOF_ARRAY_ANY, "sizeof(array) at runtime doesn't match compile-time");

