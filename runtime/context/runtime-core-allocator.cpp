// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime-common/core/runtime-core.h"
#include "runtime/allocator.h"
#include "runtime/context/runtime-context.h"

void RuntimeAllocator::init(void *buffer, size_t script_mem_size, size_t oom_handling_mem_size) {
  dl::init_script_allocator(buffer, script_mem_size, oom_handling_mem_size);
}

void RuntimeAllocator::free() {
  dl::free_script_allocator();
}

RuntimeAllocator &RuntimeAllocator::current() noexcept {
  return runtime_allocator;
}

void *RuntimeAllocator::alloc_script_memory(size_t size) noexcept {
  return dl::allocate(size);
}

void *RuntimeAllocator::alloc0_script_memory(size_t size) noexcept {
  return dl::allocate0(size);
}

void *RuntimeAllocator::realloc_script_memory(void *mem, size_t new_size, size_t old_size) noexcept {
  return dl::reallocate(mem, new_size, old_size);
}

void RuntimeAllocator::free_script_memory(void *mem, size_t size) noexcept {
  dl::deallocate(mem, size);
}

void *RuntimeAllocator::alloc_global_memory(size_t size) noexcept {
  return dl::heap_allocate(size);
}

void *RuntimeAllocator::alloc0_global_memory(size_t size) noexcept {
  void * ptr = dl::heap_allocate(size);
  if (ptr != nullptr) {
    memset(ptr, 0, size);
  }
  return ptr;
}

void *RuntimeAllocator::realloc_global_memory(void *mem, size_t new_size, size_t old_size) noexcept {
  return dl::heap_reallocate(mem, new_size, old_size);
}

void RuntimeAllocator::free_global_memory(void *mem, size_t size) noexcept {
  dl::heap_deallocate(mem, size);
}
