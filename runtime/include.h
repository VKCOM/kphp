#pragma once

#include <algorithm>

#include "runtime/php_assert.h"

template <class T>
inline void swap (T &lhs, T &rhs);


template <class T, class TT>
class array;

template <class T>
class class_instance;

template <class T>
class convert_to;

class var;

template <class T>
inline void swap (T &lhs, T &rhs) {
  const T copy_lhs (lhs);
  lhs = rhs;
  rhs = copy_lhs;
}


class Unknown {
};

template <class T>
bool eq2 (Unknown lhs __attribute__((unused)), const T &rhs __attribute__((unused))) {
  php_assert ("Comparison of Unknown" && 0);
  return false;
}

template <class T>
bool eq2 (const T &lhs __attribute__((unused)), Unknown rhs __attribute__((unused))){
  php_assert ("Comparison of Unknown" && 0);
  return false;
}

inline bool eq2 (Unknown lhs __attribute__((unused)), Unknown rhs __attribute__((unused))) {
  php_assert ("Comparison of Unknown" && 0);
  return false;
}

template <class T>
bool equals (Unknown lhs __attribute__((unused)), const T &rhs __attribute__((unused))) {
  php_assert ("Comparison of Unknown" && 0);
  return false;
}

template <class T>
bool equals (const T &lhs __attribute__((unused)), Unknown rhs __attribute__((unused))) {
  php_assert ("Comparison of Unknown" && 0);
  return false;
}

inline bool equals (Unknown lhs __attribute__((unused)), Unknown rhs __attribute__((unused))) {
  php_assert ("Comparison of Unknown" && 0);
  return true;
}


template <class T>
class OrFalse {
public:
  T value;
  bool bool_value;

  OrFalse (void): value(), bool_value() {
  }

  OrFalse (bool x): value(), bool_value (x) {
    php_assert (x == false);
  }

  template <class T1>
  OrFalse (const T1 &x): value (x), bool_value (true) {
  }

  template <class T1>
  OrFalse (const OrFalse <T1> &other): value (other.value), bool_value (other.bool_value) {
  }


  OrFalse& operator = (bool x) {
    value = T();
    bool_value = x;
    php_assert (x == false);
    return *this;
  }

  template <class T1>
  inline OrFalse& operator = (const OrFalse <T1> &other) {
    value = other.value;
    bool_value = other.bool_value;
    return *this;
  }

  template <class T1>
  inline OrFalse& operator = (const T1 &x) {
    value = x;
    bool_value = true;
    return *this;
  }


  T& ref (void) {
    bool_value = true;
    return value;
  }

  T& val (void) {
    return value;
  }

  const T& val (void) const {
    return value;
  }
};


template <>
class OrFalse <bool>;

template <>
class OrFalse <var>;

template <class T>
class OrFalse <OrFalse <T> >;


template <class T>
inline T min (const T &lhs, const T &rhs) {
  if (lhs < rhs) {
    return lhs;
  }
  return rhs;
}

template <class T>
inline T max (const T &lhs, const T &rhs) {
  if (lhs > rhs) {
    return lhs;
  }
  return rhs;
}
