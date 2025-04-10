// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <iterator>
#include <type_traits>

namespace vk {
template<class Rng>
using range_value_type = std::decay_t<typename std::iterator_traits<std::decay_t<decltype(std::begin(std::declval<const Rng&>()))>>::value_type>;
} // namespace vk
