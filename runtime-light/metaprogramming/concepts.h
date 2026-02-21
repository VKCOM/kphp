//  Compiler for PHP (aka KPHP)
//  Copyright (c) 2024 LLC «V Kontakte»
//  Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <chrono>
#include <concepts>
#include <cstddef>
#include <functional>
#include <type_traits>

namespace kphp::concepts {

template<typename T>
concept standard_layout = std::is_standard_layout_v<T>;

template<typename T>
concept hashable = requires(T t) {
  { std::hash<T>{}(t) } -> std::convertible_to<size_t>;
};

template<typename type, typename... types>
concept in_types = (std::same_as<type, types> || ...);

template<typename type>
concept duration = requires {
  typename type::rep;
  typename type::period;
} && std::is_same_v<type, std::chrono::duration<typename type::rep, typename type::period>>;

} // namespace kphp::concepts
