//  Compiler for PHP (aka KPHP)
//  Copyright (c) 2024 LLC «V Kontakte»
//  Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <concepts>
#include <cstddef>
#include <functional>

template<typename T>
concept standard_layout = std::is_standard_layout_v<T>;

template<typename T>
concept hashable = requires(T a) {
  { std::hash<T>{}(a) } -> std::convertible_to<size_t>;
};
