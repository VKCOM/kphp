// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <array>
#include <cstddef>
#include <type_traits>
#include <utility>

#include "runtime-common/runtime-core/allocator/runtime-allocator.h"

template<size_t limit>
union small_object_storage {
  std::array<std::byte, limit> storage_;
  void *storage_ptr;

  template<typename T, typename... Args>
  std::enable_if_t<sizeof(T) <= limit, T *> emplace(Args &&...args) noexcept {
    return new (storage_.data()) T(std::forward<Args>(args)...);
  }
  template<typename T>
  std::enable_if_t<sizeof(T) <= limit, T *> get() noexcept {
    return reinterpret_cast<T *>(storage_.data());
  }
  template<typename T>
  std::enable_if_t<sizeof(T) <= limit> destroy() noexcept {
    get<T>()->~T();
  }

  template<typename T, typename... Args>
    std::enable_if_t < limit<sizeof(T), T *> emplace(Args &&...args) noexcept {
    storage_ptr = RuntimeAllocator::current().alloc_script_memory(sizeof(T));
    return new (storage_ptr) T(std::forward<Args>(args)...);
  }
  template<typename T>
    std::enable_if_t < limit<sizeof(T), T *> get() noexcept {
    return static_cast<T *>(storage_ptr);
  }
  template<typename T>
    std::enable_if_t < limit<sizeof(T)> destroy() noexcept {
    T *mem = get<T>();
    mem->~T();
    RuntimeAllocator::current().free_script_memory(mem, sizeof(T));
  }
};
