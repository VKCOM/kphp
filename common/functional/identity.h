// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <type_traits>
#include <utility>

namespace vk {
struct identity {
  template<class T>
  constexpr T&& operator()(T&& t) const noexcept {
    return std::forward<T>(t);
  }
  using is_transparent = std::true_type;
};
} // namespace vk
