#pragma once

#include "runtime/kphp_core.h"

int f$bindec (const string &number);

string f$decbin (int number);

string f$dechex (int number);

int f$hexdec (const string &number);

double f$lcg_value (void);


void f$srand (int seed = INT_MIN);

int f$rand (void);

int f$rand (int r, int l);

int f$getrandmax (void);

void f$mt_srand (int seed = INT_MIN);

int f$mt_rand (void);

int f$mt_rand (int r, int l);

int f$mt_getrandmax (void);


var f$min (const var &a);

var f$max (const var &a);

template <class T>
inline T f$min (const array <T> &a);

template <class T>
inline T f$max (const array <T> &a);

template <class T>
inline T f$min (const T &v1, const T &v2);

template <class T>
inline T f$max (const T &v1, const T &v2);

template <class T>
inline T f$min (const T &v1, const T &v2, const T &v3);

template <class T>
inline T f$max (const T &v1, const T &v2, const T &v3);

template <class T>
inline T f$min (const T &v1, const T &v2, const T &v3, const T &v4);

template <class T>
inline T f$max (const T &v1, const T &v2, const T &v3, const T &v4);


const int PHP_ROUND_HALF_UP = 123423141;
const int PHP_ROUND_HALF_DOWN = 123423144;
const int PHP_ROUND_HALF_EVEN = 123423145;
const int PHP_ROUND_HALF_ODD = 123423146;

var f$abs (const var &v);

inline double f$acos (double v);

inline double f$atan (double v);

inline double f$atan2 (double y, double x);

string f$base_convert (const string &number, int frombase, int tobase);

inline double f$ceil (double v);

inline double f$cos (double v);

inline double f$deg2rad (double v);

inline double f$exp (double v);

inline double f$floor (double v);

inline double f$fmod (double x, double y);

inline bool f$is_finite (double v);

inline bool f$is_infinite (double v);

inline bool f$is_nan (double v);

inline double f$log (double v);

inline double f$log (double v, double base);

inline double f$pi (void);

var f$pow (const var &num, const var &deg);

double f$round (double v, int precision = 0);

inline double f$sin (double v);

inline double f$sqrt (double v);

inline double f$tan (double v);


/*
 *
 *     IMPLEMENTATION
 *
 */


template <class T>
T f$min (const array <T> &a) {
  if ((int)a.count() == 0) {
    php_warning ("Empty array specified to function min");
    return T();
  }

  typename array <T>::const_iterator p = a.begin();
  T res = p.get_value();
  for (++p; p != a.end(); ++p) {
    if (lt (p.get_value(), res)) {
      res = p.get_value();
    }
  }
  return res;
}

template <class T>
T f$max (const array <T> &a) {
  if ((int)a.count() == 0) {
    php_warning ("Empty array specified to function max");
    return T();
  }

  typename array <T>::const_iterator p = a.begin();
  T res = p.get_value();
  for (++p; p != a.end(); ++p) {
    if (gt (p.get_value(), res)) {
      res = p.get_value();
    }
  }
  return res;
}

template <class T>
T f$min (const T &v1, const T &v2) {
  if (lt (v1, v2)) {
    return v1;
  }
  return v2;
}

template <class T>
T f$max (const T &v1, const T &v2) {
  if (gt (v1, v2)) {
    return v1;
  }
  return v2;
}

template <class T>
T f$min (const T &v1, const T &v2, const T &v3) {
  return f$min (f$min (v1, v2), v3);
}

template <class T>
T f$max (const T &v1, const T &v2, const T &v3) {
  return f$max (f$max (v1, v2), v3);
}

template <class T>
T f$min (const T &v1, const T &v2, const T &v3, const T &v4) {
  return f$min (f$min (v1, v2), f$min (v3, v4));
}

template <class T>
T f$max (const T &v1, const T &v2, const T &v3, const T &v4) {
  return f$max (f$max (v1, v2), f$max (v3, v4));
}


double f$acos (double v) {
  return acos (v);
}

double f$atan (double v) {
  return atan (v);
}

double f$atan2 (double y, double x) {
  return atan2 (y, x);
}

double f$ceil (double v) {
  return ceil (v);
}

double f$cos (double v) {
  return cos (v);
}

double f$deg2rad (double v) {
  return v * M_PI / 180;
}

double f$exp (double v) {
  return exp (v);
}

double f$floor (double v) {
  return floor (v);
}

double f$fmod (double x, double y) {
  if (fabs (x) > 1e100 || fabs (y) < 1e-100) {
    return 0.0;
  }
  return fmod (x, y);
}

bool f$is_finite (double v) {
  int v_class = __fpclassify (v);
  return (v_class != FP_NAN && v_class != FP_INFINITE);
}

bool f$is_infinite (double v) {
  return (__fpclassify (v) == FP_INFINITE);
}

bool f$is_nan (double v) {
  return (__fpclassify (v) == FP_NAN);
}

double f$log (double v) {
  if (v <= 0.0) {
    return 0.0;
  }
  return log (v);
}

double f$log (double v, double base) {
  if (v <= 0.0 || base <= 0.0 || fabs (base - 1.0) < 1e-9) {
    return 0.0;
  }
  return log (v) / log (base);
}

double f$pi (void) {
  return M_PI;
}

double f$sin (double v) {
  return sin (v);
}

double f$sqrt (double v) {
  if (v < 0) {
    return 0.0;
  }
  return sqrt (v);
}

double f$tan (double v) {
  return tan (v);
}
