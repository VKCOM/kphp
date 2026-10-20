// Compiler for PHP (aka KPHP)
// Copyright (c) 2026 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <bit>

#include "common/containers/object-pool.h"
#include "common/mixin/not_copyable.h"
#include "runtime-common/core/allocator/platform-malloc-interface.h"
#include "runtime-common/core/allocator/script-allocator.h"
#include "runtime-common/core/memory-resource/chunk-pool-resource.h"
#include "runtime-common/core/memory-resource/stack-resource.h"
#include "runtime-light/stdlib/diagnostics/logs.h"

namespace kphp::coro::detail::memory {

namespace task {

inline auto alloc_aligned(size_t size, std::align_val_t al) noexcept -> void*;
inline auto free_aligned(void* ptr, size_t size, std::align_val_t al) noexcept -> void;

} // namespace task

struct task_allocator final : private vk::not_copyable {
private:
  vk::object_pool<memory_resource::stack_resource, kphp::memory::script_allocator<memory_resource::stack_resource>> m_stack_pool;
  memory_resource::chunk_pool_resource m_chunk_pool;
  memory_resource::stack_resource* m_curr_stack{nullptr};
  size_t m_stack_size{0};
  size_t m_min_extra_mem_size{0};

  auto request_extra_memory(size_t requested_size) noexcept -> void {
    size_t extra_mem_size{std::max(m_min_extra_mem_size, requested_size)};
    // Take into account internal layout of header for buffer
    extra_mem_size += sizeof(memory_resource::chunk_pool_resource::buffer_header_size());
    // The smallest power of two that is not smaller than `extra_mem_size`
    extra_mem_size = std::bit_ceil(extra_mem_size);

    auto* extra_mem{kphp::memory::platform::alloc(extra_mem_size)};

    kphp::log::assertion(extra_mem != nullptr);

    m_chunk_pool.add_extra_memory(extra_mem, extra_mem_size);
  }

public:
  static auto get() noexcept -> task_allocator&;

  task_allocator(size_t script_mem_size, size_t stack_size, size_t stack_pool_chunk_size, size_t min_extra_mem_size, size_t /*unused*/) noexcept
      : m_stack_pool{stack_pool_chunk_size},
        m_stack_size{stack_size},
        m_min_extra_mem_size{min_extra_mem_size} {
    void* buffer{kphp::memory::platform::alloc(script_mem_size)};

    kphp::log::assertion(buffer != nullptr);

    m_chunk_pool.init(buffer, script_mem_size, m_stack_size);
  }

  auto free() noexcept -> void {
    auto* curr_buffer{m_chunk_pool.get_buffer_list_head()};
    while (curr_buffer != nullptr) {
      auto* next_buffer = curr_buffer->next;
      kphp::memory::platform::free(curr_buffer);
      curr_buffer = next_buffer;
    }
  }

  auto current_stack() const noexcept -> memory_resource::stack_resource* {
    return m_curr_stack;
  }

  auto set_stack(memory_resource::stack_resource* stack) noexcept -> void {
    m_curr_stack = stack;
  }

  auto exchange_stack(memory_resource::stack_resource* stack) noexcept -> memory_resource::stack_resource* {
    auto* prev{m_curr_stack};
    set_stack(stack);

    return prev;
  }

private:
  auto stack_size() const noexcept -> size_t {
    return m_stack_size;
  }

  auto acquire_stack() noexcept -> memory_resource::stack_resource& {
    auto& stack{m_stack_pool.acquire()};
    auto* chunk{m_chunk_pool.allocate()};
    if (chunk == nullptr) [[unlikely]] {
      request_extra_memory(m_stack_size);
      chunk = m_chunk_pool.allocate();

      kphp::log::assertion(chunk != nullptr);
    }

    stack.init(chunk, m_stack_size);

    return stack;
  }

  auto release_stack(memory_resource::stack_resource& stack) noexcept -> void {
    m_chunk_pool.deallocate(stack.memory_begin());
    m_stack_pool.release(stack);
  }

  auto alloc_script_memory(size_t size) noexcept -> void* {
    return m_curr_stack->allocate(size);
  }

  auto free_script_memory(void* mem, size_t size) noexcept -> void {
    m_curr_stack->deallocate(mem, size);
  }

  friend inline auto kphp::coro::detail::memory::task::alloc_aligned(size_t size, std::align_val_t al) noexcept -> void*;
  friend inline auto kphp::coro::detail::memory::task::free_aligned(void* ptr, size_t size, std::align_val_t al) noexcept -> void;
};

} // namespace kphp::coro::detail::memory
