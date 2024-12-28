//  Compiler for PHP (aka KPHP)
//  Copyright (c) 2024 LLC «V Kontakte»
//  Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <concepts>
#include <cstddef>

#include "runtime-common/core/allocator/runtime-allocator.h"
#include "runtime-common/core/allocator/script-allocator-managed.h"

template<std::derived_from<ScriptAllocatorManaged> T, typename... Args>
requires std::constructible_from<T, Args...> auto make_unique_on_script_memory(Args &&...args) noexcept {
  return std::make_unique<T>(std::forward<Args>(args)...);
}

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

} // namespace memory

} // namespace kphp
