// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <algorithm>
#include <cmath>
#include <type_traits>

#include "common/wrappers/likely.h"

#include "common/php-functions.h"
#include "runtime-core/kphp-types/decl/declarations.h"
#include "runtime-core/kphp-types/decl/optional.h"
#include "runtime-core/kphp-types/kphp-type-traits.h"
#include "runtime-core/utils/kphp-assert-core.h"

#define COMMA ,


template<class T>
class convert_to {
public:
  static_assert(!std::is_same<T, int>{}, "int is forbidden");

  static inline const T &convert(const T &val);

  static inline T &&convert(T &&val);

  static inline T convert(const Unknown &val);

  template<class T1,
    class = std::enable_if_t<!std::is_same<std::decay_t<T1>, Unknown>::value>,
    class = std::enable_if_t<!std::is_same<std::decay_t<T1>, T>::value>
  >
  static inline T convert(T1 &&val);
};

template<typename Allocator>
class convert_to<__string<Allocator>> {
public:
static const __string<Allocator> convert(const __string<Allocator> &val) {
    return val;
  }

  static __string<Allocator> &&convert(__string<Allocator> &&val) {
    return std::move(val);
  }

  static __string<Allocator> convert(const Unknown &) {
    return __string<Allocator>();
  }

  template<class T1, class, class>
  static __string<Allocator> convert(T1 &&val) {
    return f$strval(std::forward<T1>(val));
  }
};

template<typename Allocator>
class convert_to<__mixed<Allocator>> {
public:
  static const __mixed<Allocator> convert(const __mixed<Allocator> &val) {
    return val;
  }

  static __mixed<Allocator> &&convert(__mixed<Allocator> &&val) {
    return std::move(val);
  }

  static __mixed<Allocator> convert(const Unknown &) {
    return __mixed<Allocator>();
  }

  template<class T1, class, class>
  static __mixed<Allocator> convert(T1 &&val) {
    return __mixed<Allocator>{std::forward<T1>(val)};
  }
};

template<typename Allocator>
class convert_to<__array<__mixed<Allocator>, Allocator>> {
public:
  static const __array<__mixed<Allocator>, Allocator> convert(const __array<__mixed<Allocator>, Allocator> &val) {
    return val;
  }

  static __array<__mixed<Allocator>, Allocator> &&convert(__array<__mixed<Allocator>, Allocator> &&val) {
    return std::move(val);
  }

  static __array<__mixed<Allocator>, Allocator> convert(const Unknown &) {
    return __array<__mixed<Allocator>, Allocator>();
  }

  template<class T1, class, class>
  static __array<__mixed<Allocator>, Allocator> convert(T1 &&val) {
    return f$arrayval(std::forward<T1>(val));
  }
};


template<class T>
inline bool f$is_null(const T &v);
template<class T, typename Allocator>
inline bool f$is_null(const __class_instance<T, Allocator> &v);
template<class T>
inline bool f$is_null(const Optional<T> &v);
template<typename Allocator>
inline bool f$is_null(const __runtime_core::mixed<Allocator> &v);

template<class T>
struct CDataPtr;

template<class T>
bool f$is_null(CDataPtr<T> ptr) { return ptr.is_php_null(); }

using std::swap;
using std::min;
using std::max;
