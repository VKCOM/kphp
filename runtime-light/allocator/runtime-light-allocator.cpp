// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include <memory>

#include "runtime-common/core/allocator/runtime-allocator.h"
#include "runtime-light/allocator/allocator-state.h"
#include "runtime-light/stdlib/diagnostics/logs.h"

auto RuntimeAllocator::get() noexcept -> RuntimeAllocator& {
  return AllocatorState::get_mutable().allocator;
}

RuntimeAllocator::RuntimeAllocator(size_t script_mem_size, size_t min_extra_mem_size, size_t oom_handling_mem_size) noexcept
    : m_allocator{script_mem_size, min_extra_mem_size, oom_handling_mem_size} {}

auto RuntimeAllocator::init(void* buffer, size_t script_mem_size, size_t oom_handling_mem_size) noexcept -> void {
  m_allocator.init(buffer, script_mem_size, oom_handling_mem_size);
}

auto RuntimeAllocator::free() noexcept -> void {
  m_allocator.free();
}

// Only the default resource may grow through pool_allocator. A replacement
// resource is borrowed and must never fall back to request-local memory.
auto RuntimeAllocator::alloc_script_memory(size_t size) noexcept -> void* {
  auto& resource{get_memory_resource()};
  if (std::addressof(resource) == std::addressof(m_allocator.get_memory_resource())) {
    return m_allocator.alloc_script_memory(size);
  }
  kphp::log::assertion(size != 0);
  void* mem{resource.allocate(size)};
  kphp::log::assertion(mem != nullptr);
  return mem;
}

auto RuntimeAllocator::calloc_script_memory(size_t size) noexcept -> void* {
  auto& resource{get_memory_resource()};
  if (std::addressof(resource) == std::addressof(m_allocator.get_memory_resource())) {
    return m_allocator.calloc_script_memory(size);
  }
  kphp::log::assertion(size != 0);
  void* mem{resource.allocate0(size)};
  kphp::log::assertion(mem != nullptr);
  return mem;
}

auto RuntimeAllocator::realloc_script_memory(void* mem, size_t new_size, size_t old_size) noexcept -> void* {
  auto& resource{get_memory_resource()};
  if (std::addressof(resource) == std::addressof(m_allocator.get_memory_resource())) {
    return m_allocator.realloc_script_memory(mem, new_size, old_size);
  }
  kphp::log::assertion(new_size > old_size);
  void* new_mem{resource.reallocate(mem, new_size, old_size)};
  kphp::log::assertion(new_mem != nullptr);
  return new_mem;
}

auto RuntimeAllocator::free_script_memory(void* mem, size_t size) noexcept -> void {
  auto& resource{get_memory_resource()};
  if (std::addressof(resource) == std::addressof(m_allocator.get_memory_resource())) {
    m_allocator.free_script_memory(mem, size);
    return;
  }
  kphp::log::assertion(size != 0);
  resource.deallocate(mem, size);
}
