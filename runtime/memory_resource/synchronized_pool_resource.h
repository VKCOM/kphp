#pragma once

#include "common/mixin/not_copyable.h"

#include "runtime/memory_resource/memory_resource.h"

namespace memory_resource {

class synchronized_pool_resource_impl;

class synchronized_pool_resource : vk::not_copyable {
public:
  void init(size_type pool_size) noexcept;
  void free() noexcept;
  void hard_reset() noexcept;

  void *allocate(size_type size) noexcept;
  void *allocate0(size_type size) noexcept;
  void deallocate(void *mem, size_type size) noexcept;
  void *reallocate(void *mem, size_type new_size, size_type old_size) noexcept;

  bool reserve(size_type size) noexcept;
  void rollback_reserve() noexcept;

  void forbid_allocations() noexcept;
  void allow_allocations() noexcept;

  bool is_reset_required() noexcept;
  MemoryStats get_memory_stats() noexcept;

private:
  void inplace_init() noexcept;

  synchronized_pool_resource_impl *impl_{nullptr};
  void *shared_memory_{nullptr};
  size_type pool_size_{0};
  size_type reserved_{0};
  bool allocations_forbidden_{false};
};

} // namespace memory_resource
