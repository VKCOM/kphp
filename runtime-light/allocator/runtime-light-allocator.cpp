// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime-common/core/allocator/runtime-allocator.h"
#include "runtime-light/allocator/allocator-state.h"

RuntimeAllocator& RuntimeAllocator::get() noexcept {
  return AllocatorState::get_mutable().allocator;
}

auto RuntimeAllocator::init(void* buffer, size_t script_mem_size, size_t oom_handling_mem_size) noexcept -> void {
  kphp::memory::details::PoolAllocator::PoolAllocator::init(buffer, script_mem_size, oom_handling_mem_size);
}

auto RuntimeAllocator::free() noexcept -> void {
  kphp::memory::details::PoolAllocator::PoolAllocator::free();
}

auto RuntimeAllocator::alloc_script_memory(size_t size) noexcept -> void* {
  return kphp::memory::details::PoolAllocator::PoolAllocator::alloc_script_memory(size);
}

auto RuntimeAllocator::alloc0_script_memory(size_t size) noexcept -> void* {
  return kphp::memory::details::PoolAllocator::PoolAllocator::alloc0_script_memory(size);
}

auto RuntimeAllocator::realloc_script_memory(void* mem, size_t new_size, size_t old_size) noexcept -> void* {
  return kphp::memory::details::PoolAllocator::PoolAllocator::realloc_script_memory(mem, new_size, old_size);
}

auto RuntimeAllocator::free_script_memory(void* mem, size_t size) noexcept -> void {
  kphp::memory::details::PoolAllocator::PoolAllocator::free_script_memory(mem, size);
}

auto RuntimeAllocator::alloc_global_memory(size_t size) noexcept -> void* {
  return kphp::memory::details::PoolAllocator::PoolAllocator::alloc_global_memory(size);
}

auto RuntimeAllocator::alloc0_global_memory(size_t size) noexcept -> void* {
  return kphp::memory::details::PoolAllocator::PoolAllocator::alloc0_global_memory(size);
}

auto RuntimeAllocator::realloc_global_memory(void* mem, size_t new_size, size_t old_size) noexcept -> void* {
  return kphp::memory::details::PoolAllocator::PoolAllocator::realloc_global_memory(mem, new_size, old_size);
}

auto RuntimeAllocator::free_global_memory(void* mem, size_t size) noexcept -> void {
  kphp::memory::details::PoolAllocator::PoolAllocator::free_global_memory(mem, size);
}
