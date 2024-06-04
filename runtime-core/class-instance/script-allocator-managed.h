#pragma once

#include <cstddef>

#include "runtime-core/runtime-core.h"

template<typename Allocator>
class ScriptAllocatorManaged {
public:
  static void *operator new(size_t size) noexcept {
    return Allocator::allocate(size);
  }

  static void *operator new(size_t, void *ptr) noexcept {
    return ptr;
  }

  static void operator delete(void *ptr, size_t size) noexcept {
    Allocator::deallocate(ptr, size);
  }

  static void *operator new[](size_t count) = delete;
  static void operator delete[](void *ptr, size_t sz) = delete;
  static void operator delete[](void *ptr) = delete;

protected:
  ~ScriptAllocatorManaged() = default;
};
