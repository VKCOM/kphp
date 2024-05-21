#pragma once

#include <cstddef>

struct CoreAllocator {
  static CoreAllocator& current() noexcept;

  void * alloc_script_memory(size_t size);
  void * alloc0_script_memory(size_t size);
  void * realloc_script_memory(void *mem, size_t new_size, size_t old_size);
  void free_script_memory(void *mem, size_t size);


  void * alloc_global_memory(size_t size);
  void * realloc_global_memory(void *mem, size_t new_size, size_t old_size);
  void free_global_memory(void *mem, size_t size);
};

class ManagedThroughAllocator {
public:
  static void *operator new(size_t size) noexcept {
    return CoreAllocator::current().alloc_script_memory(size);
  }

  static void *operator new(size_t, void *ptr) noexcept {
    return ptr;
  }

  static void operator delete(void *ptr, size_t size) noexcept {
    CoreAllocator::current().free_script_memory(ptr, size);
  }

  static void *operator new[](size_t count) = delete;
  static void operator delete[](void *ptr, size_t sz) = delete;
  static void operator delete[](void *ptr) = delete;

protected:
  ~ManagedThroughAllocator() = default;
};
