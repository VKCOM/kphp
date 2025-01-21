//  Compiler for PHP (aka KPHP)
//  Copyright (c) 2025 LLC «V Kontakte»
//  Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cstddef>

#include "runtime-common/core/allocator/runtime-allocator.h"

namespace kphp {

namespace memory {

template<typename T>
struct script_allocator {
  using value_type = T;

  constexpr value_type *allocate(size_t n) noexcept {
    return static_cast<value_type *>(RuntimeAllocator::get().alloc_script_memory(n * sizeof(T)));
  }

  constexpr void deallocate(T *p, size_t n) noexcept {
    RuntimeAllocator::get().free_script_memory(p, n);
  }
};

template<class T, class U>
constexpr bool operator==(const script_allocator<T> & /*unused*/, const script_allocator<U> & /*unused*/) {
  return true;
}

template<class T, class U>
constexpr bool operator!=(const script_allocator<T> & /*unused*/, const script_allocator<U> & /*unused*/) {
  return false;
}

} // namespace memory

} // namespace kphp
