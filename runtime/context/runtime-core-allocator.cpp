// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime-core/runtime-core.h"
#include "runtime/allocator.h"

void *RuntimeAllocator::alloc_script_memory(size_t size) {
  return dl::allocate(size);
}

void *RuntimeAllocator::alloc0_script_memory(size_t size) {
  return dl::allocate0(size);
}

void *RuntimeAllocator::realloc_script_memory(void *mem, size_t new_size, size_t old_size) {
  return dl::reallocate(mem, new_size, old_size);
}

void RuntimeAllocator::free_script_memory(void *mem, size_t size) {
  dl::deallocate(mem, size);
}

void *RuntimeAllocator::alloc_global_memory(size_t size) {
  return dl::heap_allocate(size);
}

void *RuntimeAllocator::realloc_global_memory(void *mem, size_t new_size, size_t old_size) {
  return dl::heap_reallocate(mem, new_size, old_size);
}

void RuntimeAllocator::free_global_memory(void *mem, size_t size) {
  dl::heap_deallocate(mem, size);
}
