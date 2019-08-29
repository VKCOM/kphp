#pragma once

#include <algorithm>
#include <cmath>
#include <type_traits>

#include "common/wrappers/likely.h"

#include "common-php-functions.h"
#include "runtime/declarations.h"
#include "runtime/or_false.h"
#include "runtime/php_assert.h"

#define COMMA ,


template<class T>
class convert_to {
public:
  static inline const T &convert(const T &val);

  static inline T &&convert(T &&val);

  static inline T convert(const Unknown &val);

  template<class T1,
    class = std::enable_if_t<!std::is_same<std::decay_t<T1>, Unknown>::value>,
    class = std::enable_if_t<!std::is_same<std::decay_t<T1>, T>::value>
  >
  static inline T convert(T1 &&val);
};

template<class T, class U>
struct one_of_is_unknown
  : std::integral_constant<bool, std::is_same<T, Unknown>::value || std::is_same<U, Unknown>::value> {
};

template<class T, class U>
using enable_if_one_of_types_is_unknown = typename std::enable_if<one_of_is_unknown<T, U>::value, bool>::type;

template<typename>
struct is_class_instance;

template<class T, class U>
using disable_if_one_of_types_is_unknown = typename std::enable_if<!one_of_is_unknown<T, U>::value && !(is_class_instance<T>{} && is_class_instance<U>{}), bool>::type;

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
disable_if_one_of_types_is_unknown<T, U> equals(const T &, const U &) {
  return false;
}

template<class T>
disable_if_one_of_types_is_unknown<T, T> equals(const T &lhs, const T &rhs) {
  return lhs == rhs;
}


using std::swap;
using std::min;
using std::max;
