#include <cstring>

#include "runtime-common/core/allocator/global-memory-allocator.h"
#include "runtime/allocator.h"
#include "runtime/context/runtime-context.h"

auto GlobalMemoryAllocator::get() noexcept -> GlobalMemoryAllocator& {
  return global_memory_allocator;
}

auto GlobalMemoryAllocator::alloc_global_memory(size_t size) noexcept -> void* {
  return dl::heap_allocate(size);
}

auto GlobalMemoryAllocator::alloc0_global_memory(size_t size) noexcept -> void* {
  void* ptr = dl::heap_allocate(size);
  if (ptr != nullptr) {
    memset(ptr, 0, size);
  }
  return ptr;
}

auto GlobalMemoryAllocator::realloc_global_memory(void* mem, size_t new_size, size_t old_size) noexcept -> void* {
  return dl::heap_reallocate(mem, new_size, old_size);
}

auto GlobalMemoryAllocator::free_global_memory(void* mem, size_t size) noexcept -> void {
  dl::heap_deallocate(mem, size);
}
