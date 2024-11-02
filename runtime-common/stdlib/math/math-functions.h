// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "runtime-common/core/runtime-core.h"

inline double f$ceil(double v);

inline double f$cos(double v);

inline double f$deg2rad(double v);

inline double f$floor(double v);

inline double f$log(double v);

inline double f$log(double v, double base);

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

inline double f$pi();

inline double f$round(double v, int64_t precision = 0);

inline double f$sqrt(double v);

mixed f$abs(const mixed &v);
int64_t f$abs(int64_t v);
double f$abs(double v);
int64_t f$abs(const Optional<int64_t> &v);
int64_t f$abs(const Optional<bool> &v);
double f$abs(const Optional<double> &v);

inline double f$ceil(double v) {
  return ceil(v);
}

inline double f$cos(double v) {
  return cos(v);
}

inline double f$deg2rad(double v) {
  return v * M_PI / 180;
}

inline double f$floor(double v) {
  return floor(v);
}

inline double f$log(double v) {
  if (v <= 0.0) {
    return 0.0;
  }
  return log(v);
}

inline double f$log(double v, double base) {
  if (v <= 0.0 || base <= 0.0 || fabs(base - 1.0) < 1e-9) {
    return 0.0;
  }
  return log(v) / log(base);
}

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

inline double f$pi() {
  return M_PI;
}

inline double f$round(double v, int64_t precision) {
  if (std::abs(precision) > 100) {
    php_warning("Wrong parameter precision (%" PRIi64 ") in function round", precision);
    return v;
  }

  double mul = pow(10.0, (double)precision);
  return round(v * mul) / mul;
}

inline double f$sqrt(double v) {
  if (v < 0) {
    return 0.0;
  }
  return sqrt(v);
}