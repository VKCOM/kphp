//  Compiler for PHP (aka KPHP)
//  Copyright (c) 2025 LLC «V Kontakte»
//  Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cstddef>

#include "runtime-common/core/allocator/runtime-allocator.h"

namespace kphp::memory {

template<typename T>
struct platform_allocator {
  using value_type = T;

  constexpr value_type *allocate(size_t n) noexcept {
    return static_cast<value_type *>(RuntimeAllocator::get().alloc_global_memory(n * sizeof(T)));
  }

  constexpr void deallocate(T *p, size_t n) noexcept {
    RuntimeAllocator::get().free_global_memory(p, n * sizeof(T));
  }
};

template<class T, class U>
constexpr bool operator==(const platform_allocator<T> & /*unused*/, const platform_allocator<U> & /*unused*/) {
  return true;
}

template<class T, class U>
constexpr bool operator!=(const platform_allocator<T> & /*unused*/, const platform_allocator<U> & /*unused*/) {
  return false;
}

} // namespace kphp::memory
