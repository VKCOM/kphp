// Compiler for PHP (aka KPHP)
// Copyright (c) 2022 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <type_traits>

#include "runtime-common/core/runtime-core.h"

namespace impl_ {
template<typename T, typename = std::void_t<>>
struct IsJsonFlattenClass : std::false_type {};

template<typename T>
struct IsJsonFlattenClass<T, std::enable_if_t<T::json_flatten_class>> : std::true_type {};

template<typename T, typename = std::void_t<>>
struct HasClassWakeupMethod : std::false_type {};

template<typename T>
struct HasClassWakeupMethod<T, std::enable_if_t<T::has_wakeup_method>> : std::true_type {};
} // namespace impl_

struct JsonRawString {
  explicit JsonRawString(string &s) noexcept
    : str(s) {}
  string &str;
};
