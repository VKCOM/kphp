// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <type_traits>

namespace vk {

template<typename T>
struct dependent_false : std::false_type {};

template<typename T>
inline constexpr bool dependent_false_v = dependent_false<T>::value;

} // namespace vk
