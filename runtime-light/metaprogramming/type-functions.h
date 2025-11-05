//  Compiler for PHP (aka KPHP)
//  Copyright (c) 2025 LLC «V Kontakte»
//  Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cstddef>
#include <type_traits>
#include <variant>

namespace kphp::type_functions {

template<typename variant_type, typename T, size_t index = 0>
requires(index < std::variant_size_v<variant_type>)
consteval size_t variant_index() noexcept {
  if constexpr (std::is_same_v<std::variant_alternative_t<index, variant_type>, T>) {
    return index;
  } else {
    return variant_index<variant_type, T, index + 1>();
  }
}

} // namespace kphp::type_functions
