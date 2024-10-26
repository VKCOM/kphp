// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <algorithm>
#include <cmath>
#include <type_traits>

#include "common/wrappers/likely.h"

#include "common/php-functions.h"
#include "runtime-common/core/core-types/decl/declarations.h"
#include "runtime-common/core/core-types/kphp_type_traits.h"
#include "runtime-common/core/core-types/decl/optional.h"
#include "runtime-common/core/utils/kphp-assert-core.h"

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

template<class T>
inline bool f$is_null(const T &v);
template<class T>
inline bool f$is_null(const class_instance<T> &v);
template<class T>
inline bool f$is_null(const Optional<T> &v);
inline bool f$is_null(const mixed &v);

template<class T>
struct CDataPtr;

template<class T>
bool f$is_null(CDataPtr<T> ptr) { return ptr.is_php_null(); }

using std::swap;
using std::min;
using std::max;
