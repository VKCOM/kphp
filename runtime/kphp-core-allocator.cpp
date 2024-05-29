#include "kphp-core/kphp_core.h"

#include "runtime/allocator.h"

void *CoreAllocator::alloc_script_memory(size_t size) {
  return dl::allocate(size);
}

void *CoreAllocator::alloc0_script_memory(size_t size) {
  return dl::allocate0(size);
}

void *CoreAllocator::realloc_script_memory(void *mem, size_t new_size, size_t old_size) {
  return dl::reallocate(mem, new_size, old_size);
}

void CoreAllocator::free_script_memory(void *mem, size_t size) {
  dl::deallocate(mem, size);
}

void *CoreAllocator::alloc_global_memory(size_t size) {
  return dl::heap_allocate(size);
}

void *CoreAllocator::realloc_global_memory(void *mem, size_t new_size, size_t old_size) {
  return dl::heap_reallocate(mem, new_size, old_size);
}

void CoreAllocator::free_global_memory(void *mem, size_t size) {
  dl::heap_deallocate(mem, size);
}
