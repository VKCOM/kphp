// Compiler for PHP (aka KPHP)
// Copyright (c) 2026 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cstddef>

#include "runtime-common/core/allocator/pool-allocator.h"
#include "runtime-light/coroutine/coroutine-state.h"

namespace kphp::coro::detail::memory {

struct RuntimeCoroutineAllocator final {
private:
  kphp::memory::pool_allocator m_allocator;

public:
  static auto get() noexcept -> RuntimeCoroutineAllocator& {
    return kphp::coro::instance_state::get().coroutine_allocator;
  }

  RuntimeCoroutineAllocator() = default;

  RuntimeCoroutineAllocator(size_t script_mem_size, size_t min_extra_mem_size, size_t oom_handling_mem_size) noexcept
      : m_allocator{script_mem_size, min_extra_mem_size, oom_handling_mem_size} {}

  auto init(void* buffer, size_t script_mem_size, size_t oom_handling_mem_size) noexcept -> void {
    m_allocator.init(buffer, script_mem_size, oom_handling_mem_size);
  }

  auto free() noexcept -> void {
    m_allocator.free();
  }

  auto alloc_script_memory(size_t size) noexcept -> void* {
    return m_allocator.alloc(size);
  }

  auto calloc_script_memory(size_t size) noexcept -> void* {
    return m_allocator.calloc(size);
  }

  auto realloc_script_memory(void* mem, size_t new_size, size_t old_size) noexcept -> void* {
    return m_allocator.realloc(mem, new_size, old_size);
  }

  auto free_script_memory(void* mem, size_t size) noexcept -> void {
    m_allocator.free(mem, size);
  }

  auto get_memory_resource() noexcept -> memory_resource::unsynchronized_pool_resource& {
    return m_allocator.get_memory_resource();
  }
};

} // namespace kphp::coro::detail::memory
