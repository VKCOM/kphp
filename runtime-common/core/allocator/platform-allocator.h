//  Compiler for PHP (aka KPHP)
//  Copyright (c) 2025 LLC «V Kontakte»
//  Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cstddef>
#include <type_traits>

#include "runtime-common/core/allocator/platform-malloc-interface.h"

namespace kphp::memory {

template<typename T>
struct platform_allocator {
  using value_type = T;
  using propagate_on_container_copy_assignment = std::true_type;
  using propagate_on_container_move_assignment = std::true_type;
  using is_always_equal = std::true_type;

  platform_allocator() noexcept = default;

  template<typename U>
  explicit platform_allocator(const platform_allocator<U>& /*unused*/) noexcept {}

  template<typename U>
  struct rebind {
    using other = platform_allocator<U>;
  };

  constexpr value_type* allocate(size_t n) noexcept {
    return static_cast<value_type*>(kphp::memory::platform::alloc(n * sizeof(T)));
  }

  constexpr void deallocate(T* p, size_t /*unused*/) noexcept {
    kphp::memory::platform::free(p);
  }
};

template<class T, class U>
constexpr bool operator==(const platform_allocator<T>& /*unused*/, const platform_allocator<U>& /*unused*/) {
  return true;
}

template<class T, class U>
constexpr bool operator!=(const platform_allocator<T>& /*unused*/, const platform_allocator<U>& /*unused*/) {
  return false;
}

} // namespace kphp::memory
