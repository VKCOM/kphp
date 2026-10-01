//  Compiler for PHP (aka KPHP)
//  Copyright (c) 2026 LLC «V Kontakte»
//  Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cstddef>

#include "runtime-light/coroutine/detail/allocator/runtime-coroutine-allocator.h"

namespace kphp::coro::detail::memory {

template<typename T>
struct coroutine_allocator {
  using value_type = T;

  coroutine_allocator() noexcept = default;

  template<typename U>
  explicit coroutine_allocator(const coroutine_allocator<U>& /*unused*/) noexcept {}

  constexpr value_type* allocate(size_t n) noexcept {
    return static_cast<value_type*>(kphp::coro::detail::memory::runtime_coroutine_allocator::get().alloc_script_memory(n * sizeof(T)));
  }

  constexpr void deallocate(T* p, size_t n) noexcept {
    kphp::coro::detail::memory::runtime_coroutine_allocator::get().free_script_memory(p, n * sizeof(T));
  }
};

template<class T, class U>
constexpr bool operator==(const coroutine_allocator<T>& /*unused*/, const coroutine_allocator<U>& /*unused*/) {
  return true;
}

template<class T, class U>
constexpr bool operator!=(const coroutine_allocator<T>& /*unused*/, const coroutine_allocator<U>& /*unused*/) {
  return false;
}

} // namespace kphp::coro::detail::memory
