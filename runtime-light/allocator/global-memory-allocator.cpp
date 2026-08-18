#include "runtime-common/core/allocator/global-memory-allocator.h"
#include "runtime-light/k2-platform/k2-api.h"
#include "runtime-light/stdlib/diagnostics/logs.h"

auto GlobalMemoryAllocator::alloc_global_memory(size_t size) noexcept -> void* {
  void* mem{k2::alloc(size)};
  kphp::log::assertion(mem != nullptr);
  return mem;
}

auto GlobalMemoryAllocator::alloc0_global_memory(size_t size) noexcept -> void* {
  void* mem{k2::alloc(size)};
  kphp::log::assertion(mem != nullptr);
  std::memset(mem, 0, size);
  return mem;
}

auto GlobalMemoryAllocator::realloc_global_memory(void* old_mem, size_t new_size, size_t /*unused*/) noexcept -> void* {
  void* mem{k2::realloc(old_mem, new_size)};
  kphp::log::assertion(mem != nullptr);
  return mem;
}

auto GlobalMemoryAllocator::free_global_memory(void* mem, size_t /*unused*/) noexcept -> void {
  k2::free(mem);
}
