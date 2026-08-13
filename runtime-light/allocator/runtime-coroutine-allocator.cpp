#include "runtime-light/allocator/runtime-coroutine-allocator.h"

RuntimeCoroutineAllocator::RuntimeCoroutineAllocator(size_t script_mem_size, size_t min_extra_mem_size, size_t oom_handling_mem_size) noexcept
    : m_allocator{script_mem_size, min_extra_mem_size, oom_handling_mem_size} {}

auto RuntimeCoroutineAllocator::init(void* buffer, size_t script_mem_size, size_t oom_handling_mem_size) noexcept -> void {
  m_allocator.init(buffer, script_mem_size, oom_handling_mem_size);
}

auto RuntimeCoroutineAllocator::free() noexcept -> void {
  m_allocator.free();
}

auto RuntimeCoroutineAllocator::alloc_script_memory(size_t size) noexcept -> void* {
  return m_allocator.alloc_script_memory(size);
}

auto RuntimeCoroutineAllocator::alloc0_script_memory(size_t size) noexcept -> void* {
  return m_allocator.alloc0_script_memory(size);
}

auto RuntimeCoroutineAllocator::realloc_script_memory(void* mem, size_t new_size, size_t old_size) noexcept -> void* {
  return m_allocator.realloc_script_memory(mem, new_size, old_size);
}

auto RuntimeCoroutineAllocator::free_script_memory(void* mem, size_t size) noexcept -> void {
  m_allocator.free_script_memory(mem, size);
}

auto RuntimeCoroutineAllocator::alloc_global_memory(size_t size) noexcept -> void* {
  return m_allocator.alloc_global_memory(size);
}

auto RuntimeCoroutineAllocator::alloc0_global_memory(size_t size) noexcept -> void* {
  return m_allocator.alloc0_global_memory(size);
}

auto RuntimeCoroutineAllocator::realloc_global_memory(void* mem, size_t new_size, size_t old_size) noexcept -> void* {
  return m_allocator.realloc_global_memory(mem, new_size, old_size);
}

auto RuntimeCoroutineAllocator::free_global_memory(void* mem, size_t size) noexcept -> void {
  m_allocator.free_global_memory(mem, size);
}
