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
#include "runtime-common/core/memory-resource/segmented-stack-resource.h"
#include "runtime-light/stdlib/diagnostics/logs.h"

namespace kphp::coro::detail::memory {

namespace task {

inline auto alloc_aligned(size_t size, std::align_val_t al) noexcept -> void*;
inline auto free_aligned(void* ptr, size_t size, std::align_val_t al) noexcept -> void;

} // namespace task

struct task_allocator final : private vk::not_copyable {
  struct shared_chunk_pool {
    memory_resource::chunk_pool_resource& m_chunk_pool;

    shared_chunk_pool() noexcept
        : m_chunk_pool{task_allocator::get().m_chunk_pool} {}

    auto init(void* /*unused*/, size_t /*unused*/, size_t /*unused*/) noexcept -> void {}

    auto allocate() noexcept -> void* {
      return m_chunk_pool.allocate();
    }

    auto allocate0() noexcept -> void* {
      return m_chunk_pool.allocate0();
    }

    auto deallocate(void* mem) noexcept -> void {
      m_chunk_pool.deallocate(mem);
    }

    auto add_extra_memory(void* buffer, size_t buffer_size) noexcept -> void {
      m_chunk_pool.add_extra_memory(buffer, buffer_size);
    }

    auto get_buffer_list_head() const noexcept -> auto* {
      return m_chunk_pool.get_buffer_list_head();
    }
  };

private:
  vk::object_pool<memory_resource::segmented_stack_resource<shared_chunk_pool>,
                  kphp::memory::script_allocator<memory_resource::segmented_stack_resource<shared_chunk_pool>>>
      m_stack_pool;
  memory_resource::chunk_pool_resource m_chunk_pool;
  memory_resource::segmented_stack_resource<shared_chunk_pool>* m_curr_stack{nullptr};
  size_t m_segment_size{0};
  size_t m_min_extra_mem_size{0};

  size_t m_chain_count{0};
  size_t m_active_chains{0};
  size_t m_max_active_chains{0};
  size_t m_max_chain_depth{0};
  size_t m_sum_max_depth{0};
  size_t m_max_bytes_per_chain{0};
  size_t m_sum_max_bytes_per_chain{0};
  size_t m_frame_count{0};
  size_t m_max_frame_size{0};
  size_t m_sum_frame_size{0};
  size_t m_extra_mem_requests{0};
  size_t m_coroutine_pool_frames{0};

  auto request_extra_memory(size_t requested_size) noexcept -> void {
    ++m_extra_mem_requests;

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

  task_allocator(size_t script_mem_size, size_t segment_size, size_t stack_pool_chunk_size, size_t min_extra_mem_size, size_t /*unused*/) noexcept
      : m_stack_pool{stack_pool_chunk_size},
        m_segment_size{segment_size},
        m_min_extra_mem_size{min_extra_mem_size} {
    void* buffer{kphp::memory::platform::alloc(script_mem_size)};

    kphp::log::assertion(buffer != nullptr);

    m_chunk_pool.init(buffer, script_mem_size, m_segment_size + memory_resource::segmented_stack_resource<shared_chunk_pool>::segment_header_size());
  }

  auto free() noexcept -> void {
    auto* curr_buffer{m_chunk_pool.get_buffer_list_head()};
    while (curr_buffer != nullptr) {
      auto* next_buffer = curr_buffer->next;
      kphp::memory::platform::free(curr_buffer);
      curr_buffer = next_buffer;
    }
  }

  auto acquire_stack() noexcept -> memory_resource::segmented_stack_resource<shared_chunk_pool>& {
    auto& stack{m_stack_pool.acquire()};
    // We can pass nullptr as buffer and 0 as buffer_size, because init in shared_chunk_pool is noop
    stack.init(nullptr, 0, m_segment_size);

    ++m_active_chains;
    m_max_active_chains = std::max(m_max_active_chains, m_active_chains);

    return stack;
  }

  auto release_stack(memory_resource::segmented_stack_resource<shared_chunk_pool>& stack) noexcept -> void {
    ++m_chain_count;
    --m_active_chains;
    m_max_chain_depth = std::max(m_max_chain_depth, stack.max_depth());
    m_sum_max_depth += stack.max_depth();
    m_max_bytes_per_chain = std::max(m_max_bytes_per_chain, stack.max_bytes_in_use());
    m_sum_max_bytes_per_chain += stack.max_bytes_in_use();

    m_stack_pool.release(stack);
  }

  auto segment_size() const noexcept -> size_t {
    return m_segment_size;
  }

  auto current_stack() const noexcept -> memory_resource::segmented_stack_resource<shared_chunk_pool>* {
    return m_curr_stack;
  }

  auto set_stack(memory_resource::segmented_stack_resource<shared_chunk_pool>* stack) noexcept -> void {
    m_curr_stack = stack;
  }

  auto exchange_stack(memory_resource::segmented_stack_resource<shared_chunk_pool>* stack) noexcept
      -> memory_resource::segmented_stack_resource<shared_chunk_pool>* {
    auto* prev{m_curr_stack};
    set_stack(stack);

    return prev;
  }

  auto log_stats() const noexcept -> void {
    const double avg_max_chain_depth{m_chain_count != 0 ? static_cast<double>(m_sum_max_depth) / static_cast<double>(m_chain_count) : 0.0};
    const double avg_max_bytes_per_chain{m_chain_count != 0 ? static_cast<double>(m_sum_max_bytes_per_chain) / static_cast<double>(m_chain_count) : 0.0};
    const double avg_frame_size{m_frame_count != 0 ? static_cast<double>(m_sum_frame_size) / static_cast<double>(m_frame_count) : 0.0};

    kphp::log::info("coro stats: chains={} max_active_chains={} max_chain_depth={} avg_max_chain_depth={} "
                     "max_bytes_per_chain={} avg_max_bytes_per_chain={} "
                     "frames={} max_frame_size={} avg_frame_size={} "
                     "coroutine_pool_frames={} extra_mem_requests={}",
                     m_chain_count, m_max_active_chains, m_max_chain_depth, avg_max_chain_depth, m_max_bytes_per_chain, avg_max_bytes_per_chain,
                     m_frame_count, m_max_frame_size, avg_frame_size, m_coroutine_pool_frames, m_extra_mem_requests);
  }

private:
  auto alloc_script_memory(size_t size) noexcept -> void* {
    void* mem{m_curr_stack->allocate(size)};
    if (mem == nullptr) [[unlikely]] {
      request_extra_memory(size);
      mem = m_curr_stack->allocate(size);

      kphp::log::assertion(mem != nullptr);
    }

    return mem;
  }

  auto free_script_memory(void* mem, size_t size) noexcept -> void {
    m_curr_stack->deallocate(mem, size);
  }

  auto record_frame(size_t size, bool coroutine_pool_backend) noexcept -> void {
    ++m_frame_count;
    m_max_frame_size = std::max(m_max_frame_size, size);
    m_sum_frame_size += size;
    if (coroutine_pool_backend) {
      ++m_coroutine_pool_frames;
    }
  }

  friend inline auto kphp::coro::detail::memory::task::alloc_aligned(size_t size, std::align_val_t al) noexcept -> void*;
  friend inline auto kphp::coro::detail::memory::task::free_aligned(void* ptr, size_t size, std::align_val_t al) noexcept -> void;
};

} // namespace kphp::coro::detail::memory
