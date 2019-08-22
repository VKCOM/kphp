#pragma once

#include "common/mixin/not_copyable.h"

#include "runtime/memory_resource/memory_resource.h"

namespace memory_resource {

class synchronized_pool_resource_impl;

class synchronized_pool_resource : vk::not_copyable {
public:
  void init(size_type pool_size);
  void free();
  void hard_reset();

  void *allocate(size_type size);
  void *allocate0(size_type size) { return memset(allocate(size), 0x00, size); }
  void deallocate(void *mem, size_type size);
  void *reallocate(void *mem, size_type new_size, size_type old_size);

  bool reserve(size_type size);
  void rollback_reserve();

  void forbid_allocations();
  void allow_allocations();

  MemoryStats get_memory_stats();

private:
  void inplace_init();

  synchronized_pool_resource_impl *impl_{nullptr};
  void *shared_memory_{nullptr};
  size_type pool_size_{0};
  size_type reserved_{0};
  bool allocations_forbidden_{false};
};

} // namespace memory_resource
