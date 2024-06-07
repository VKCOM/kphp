#pragma once

#include <cstddef>

#include "runtime-core/runtime-core.h"

class ScriptAllocatorManaged {
public:
  static void *operator new(size_t size) noexcept {
    return RuntimeAllocator::current().alloc_script_memory(size);
  }

  static void *operator new(size_t, void *ptr) noexcept {
    return ptr;
  }

  static void operator delete(void *ptr, size_t size) noexcept {
    RuntimeAllocator::current().free_script_memory(ptr, size);
  }

  static void *operator new[](size_t count) = delete;
  static void operator delete[](void *ptr, size_t sz) = delete;
  static void operator delete[](void *ptr) = delete;

protected:
  ~ScriptAllocatorManaged() = default;
};
