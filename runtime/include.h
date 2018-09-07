#pragma once

#include <algorithm>
#include <type_traits>
#include <cmath>

#include "runtime/php_assert.h"
#include "common-php-functions.h"
#include "common/wrappers/likely.h"

#define COMMA ,


struct array_size;

class array_tag {};

template <class T>
class array;

template <class T>
class class_instance;

class var;

class Unknown {};

template <class T, class TT = T>
class force_convert_to;

class string;

class string_buffer;

template <class T>
class convert_to {
public:
  static inline const T& convert (const T &val);

  static inline T convert (const Unknown &val);

  template <class T1>
  static inline T convert (const T1 &val);
};

template<class T, class U>
struct one_of_is_unknown
  : std::integral_constant<bool, std::is_same<T, Unknown>::value || std::is_same<U, Unknown>::value> {
};

template<class T, class U>
using enable_if_one_of_types_is_unknown = typename std::enable_if<one_of_is_unknown<T, U>::value, bool>::type;

template<class T, class U>
using disable_if_one_of_types_is_unknown = typename std::enable_if<!one_of_is_unknown<T, U>::value, bool>::type;

template<class T, class U>
enable_if_one_of_types_is_unknown<T, U> eq2(const T &, const U &) {
  php_assert ("Comparison of Unknown" && 0);
  return false;
}

template<class T, class U>
enable_if_one_of_types_is_unknown<T, U> equals(const T &lhs, const U &rhs) {
  return eq2(lhs, rhs);
}

template<class T, class U>
disable_if_one_of_types_is_unknown<T, U> equals(const T &lhs, const U &rhs) {
  return false;
}

template<class T>
disable_if_one_of_types_is_unknown<T, T> equals(const T &lhs, const T &rhs) {
  return lhs == rhs;
}

class OrFalseTag {};

template <class T>
class OrFalse : public OrFalseTag {
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

template<class T>
using enable_if_t_is_or_false = typename std::enable_if<std::is_base_of<OrFalseTag, T>::value>::type;

template<class T, class T2>
using enable_if_t_is_or_false_t2 = typename std::enable_if<std::is_same<T, OrFalse<T2>>::value>::type;

template<class T>
using enable_if_t_is_or_false_string = enable_if_t_is_or_false_t2<T, string>;

template <>
class OrFalse <bool>;

template <>
class OrFalse <var>;

template <class T>
class OrFalse <OrFalse <T> >;

using std::swap;
using std::min;
using std::max;
