//  Compiler for PHP (aka KPHP)
//  Copyright (c) 2024 LLC «V Kontakte»
//  Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cstddef>

#include "runtime-common/core/allocator/runtime-allocator.h"

class ScriptAllocatorManaged {
public:
  static void *operator new(size_t size) noexcept {
    return RuntimeAllocator::get().alloc_script_memory(size);
  }

  static void *operator new(size_t, void *ptr) noexcept {
    return ptr;
  }

  static void operator delete(void *ptr, size_t size) noexcept {
    RuntimeAllocator::get().free_script_memory(ptr, size);
  }

  static void *operator new[](size_t count) = delete;
  static void operator delete[](void *ptr, size_t sz) = delete;
  static void operator delete[](void *ptr) = delete;

protected:
  ~ScriptAllocatorManaged() = default;
};
