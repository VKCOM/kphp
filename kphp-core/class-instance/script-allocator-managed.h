#pragma once

#include <cstddef>

#include "kphp-core/kphp_core.h"

class ScriptAllocatorManaged {
public:
  static void *operator new(size_t size) noexcept {
    return KphpCoreContext::current().allocator.alloc_script_memory(size);
  }

  static void *operator new(size_t, void *ptr) noexcept {
    return ptr;
  }

  static void operator delete(void *ptr, size_t size) noexcept {
    KphpCoreContext::current().allocator.free_script_memory(ptr, size);
  }

  static void *operator new[](size_t count) = delete;
  static void operator delete[](void *ptr, size_t sz) = delete;
  static void operator delete[](void *ptr) = delete;

protected:
  ~ScriptAllocatorManaged() = default;
};
