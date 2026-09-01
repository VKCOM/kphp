// Compiler for PHP (aka KPHP)
// Copyright (c) 2026 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cstddef>

#include "runtime-common/core/allocator/pool-allocator.h"

namespace kphp::coro::detail::memory {

struct runtime_coroutine_allocator final {
private:
  kphp::memory::pool_allocator m_allocator;

public:
  static auto get() noexcept -> runtime_coroutine_allocator&;

  runtime_coroutine_allocator() = default;
  runtime_coroutine_allocator(size_t script_mem_size, size_t min_extra_mem_size, size_t oom_handling_mem_size) noexcept
      : m_allocator{script_mem_size, min_extra_mem_size, oom_handling_mem_size} {}

  auto init(void* buffer, size_t script_mem_size, size_t oom_handling_mem_size) noexcept -> void {
    m_allocator.init(buffer, script_mem_size, oom_handling_mem_size);
  }

  auto free() noexcept -> void {
    m_allocator.free();
  }

  auto alloc_script_memory(size_t size) noexcept -> void* {
    return m_allocator.alloc_script_memory(size);
  }

  auto calloc_script_memory(size_t size) noexcept -> void* {
    return m_allocator.calloc_script_memory(size);
  }

  auto realloc_script_memory(void* mem, size_t new_size, size_t old_size) noexcept -> void* {
    return m_allocator.realloc_script_memory(mem, new_size, old_size);
  }

  auto free_script_memory(void* mem, size_t size) noexcept -> void {
    m_allocator.free_script_memory(mem, size);
  }
};

} // namespace kphp::coro::detail::memory
