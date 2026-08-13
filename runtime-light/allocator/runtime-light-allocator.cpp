// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime-common/core/allocator/runtime-allocator.h"
#include "runtime-light/allocator/allocator-state.h"

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

auto RuntimeAllocator::alloc_script_memory(size_t size) noexcept -> void* {
  return m_allocator.alloc_script_memory(size);
}

auto RuntimeAllocator::alloc0_script_memory(size_t size) noexcept -> void* {
  return m_allocator.alloc0_script_memory(size);
}

auto RuntimeAllocator::realloc_script_memory(void* mem, size_t new_size, size_t old_size) noexcept -> void* {
  return m_allocator.realloc_script_memory(mem, new_size, old_size);
}

auto RuntimeAllocator::free_script_memory(void* mem, size_t size) noexcept -> void {
  m_allocator.free_script_memory(mem, size);
}

auto RuntimeAllocator::alloc_global_memory(size_t size) noexcept -> void* {
  return m_allocator.alloc_global_memory(size);
}

auto RuntimeAllocator::alloc0_global_memory(size_t size) noexcept -> void* {
  return m_allocator.alloc0_global_memory(size);
}

auto RuntimeAllocator::realloc_global_memory(void* mem, size_t new_size, size_t old_size) noexcept -> void* {
  return m_allocator.realloc_global_memory(mem, new_size, old_size);
}

auto RuntimeAllocator::free_global_memory(void* mem, size_t size) noexcept -> void {
  m_allocator.free_global_memory(mem, size);
}

auto RuntimeAllocator::get_memory_resource() noexcept -> memory_resource::unsynchronized_pool_resource& {
  return m_allocator.memory_resource;
}
