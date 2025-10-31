//  Compiler for PHP (aka KPHP)
//  Copyright (c) 2025 LLC «V Kontakte»
//  Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cstddef>
#include <variant>

namespace kphp::type_traits {

template<typename variant_type, typename T, size_t index = 0>
constexpr size_t variant_index() noexcept {
  static_assert(index < std::variant_size_v<variant_type>, "Type not found in variant");
  if constexpr (index == std::variant_size_v<variant_type>) {
    // To prevent the error of substituting an index that is too large in variant_alternative_t
    return index;
  } else if constexpr (std::is_same_v<std::variant_alternative_t<index, variant_type>, T>) {
    return index;
  } else {
    return variant_index<variant_type, T, index + 1>();
  }
}

} // namespace kphp::type_traits
