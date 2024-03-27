// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cstddef>
#include <cstdlib>
#include <memory>

#include "common/containers/final_action.h"
#include "runtime-light/allocator/memory_resource/memory_resource.h"
#include "runtime-light/allocator/memory_resource/unsynchronized_pool_resource.h"

namespace memory_resource {
class unsynchronized_pool_resource;
}

namespace dl {
struct ScriptAllocator;

const memory_resource::MemoryStats &get_script_memory_stats() noexcept;

void init_script_allocator(ScriptAllocator * script_allocator, size_t script_mem_size, size_t oom_handling_mem_size) noexcept;
void free_script_allocator(ScriptAllocator * script_allocator) noexcept;

void *allocate(size_t n) noexcept;                                    // allocate script memory
void *allocate0(size_t n) noexcept;                                   // allocate zeroed script memory
void *reallocate(void *p, size_t new_size, size_t old_size) noexcept; // reallocate script memory
void deallocate(void *p, size_t n) noexcept;                          // deallocate script memory

struct ScriptAllocator {
  ScriptAllocator() {dl::init_script_allocator(this, 16 * 1024u, 0);}
  ~ScriptAllocator() {dl::free_script_allocator(this);}

  memory_resource::unsynchronized_pool_resource memory_resource;
};

class ManagedThroughDlAllocator {
public:
  static void *operator new(size_t size) noexcept {
    return dl::allocate(size);
  }

  static void *operator new(size_t, void *ptr) noexcept {
    return ptr;
  }

  static void operator delete(void *ptr, size_t size) noexcept {
    dl::deallocate(ptr, size);
  }

  static void *operator new[](size_t count) = delete;
  static void operator delete[](void *ptr, size_t sz) = delete;
  static void operator delete[](void *ptr) = delete;

protected:
  ~ManagedThroughDlAllocator() = default;
};
} // namespace dl

