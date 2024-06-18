#pragma once

#include "runtime-light/allocator/allocator.h"
#include "runtime-light/core/runtime_types/storage.h"

//template<size_t limit>
//union small_object_storage {
//  char storage_[limit];
//  void *storage_ptr;
//
//  template <typename T, typename ...Args>
//  std::enable_if_t<sizeof(T) <= limit, T*> emplace(Args&& ...args) noexcept {
//    return new (storage_) T(std::forward<Args>(args)...);
//  }
//  template <typename T>
//  std::enable_if_t<sizeof(T) <= limit, T*> get() noexcept {
//    return reinterpret_cast<T*>(storage_);
//  }
//  template <typename T>
//  std::enable_if_t<sizeof(T) <= limit> destroy() noexcept {
//    get<T>()->~T();
//  }
//
//  template <typename T, typename ...Args>
//    std::enable_if_t<limit < sizeof(T), T*> emplace(Args&& ...args) noexcept {
//    storage_ptr = dl::allocate(sizeof(T));
//    return new (storage_ptr) T(std::forward<Args>(args)...);
//  }
//  template <typename T>
//    std::enable_if_t<limit < sizeof(T), T*> get() noexcept {
//    return static_cast<T*>(storage_ptr);
//  }
//  template <typename T>
//    std::enable_if_t<limit < sizeof(T)> destroy() noexcept {
//    T *mem = get<T>();
//    mem->~T();
//    dl::deallocate(mem, sizeof(T));
//  }
//};