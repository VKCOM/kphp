// Compiler for PHP (aka KPHP)
// Copyright (c) 2026 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <bit>

#include "common/mixin/not_copyable.h"
#include "runtime-common/core/allocator/platform-malloc-interface.h"
#include "runtime-common/core/memory-resource/chunk-pool-resource.h"
#include "runtime-common/core/memory-resource/segmented-stack-resource.h"
#include "runtime-light/stdlib/diagnostics/logs.h"

namespace kphp::coro::detail::memory {

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
  memory_resource::chunk_pool_resource m_chunk_pool;
  memory_resource::segmented_stack_resource<shared_chunk_pool>* m_curr_resource{nullptr};
  size_t m_segment_size{0};
  size_t m_min_extra_mem_size{0};

  auto request_extra_memory(size_t requested_size) noexcept -> void {
    size_t extra_mem_size{std::max(m_min_extra_mem_size, requested_size)};
    // Take into account internal layout of `memory_resource::buffer_list_node`
    extra_mem_size += sizeof(memory_resource::buffer_list_node);
    // The smallest power of two that is not smaller than `extra_mem_size`
    extra_mem_size = std::bit_ceil(extra_mem_size);

    auto* extra_mem{kphp::memory::platform::alloc(extra_mem_size)};

    kphp::log::assertion(extra_mem != nullptr);

    m_curr_resource->add_extra_memory(extra_mem, extra_mem_size);
  }

public:
  static auto get() noexcept -> task_allocator&;

  task_allocator() = default;

  task_allocator(size_t script_mem_size, size_t segment_size, size_t min_extra_mem_size, size_t oom_handling_mem_size) noexcept
      : m_min_extra_mem_size{min_extra_mem_size} {
    void* buffer{kphp::memory::platform::alloc(script_mem_size)};

    kphp::log::assertion(buffer != nullptr);

    init(buffer, script_mem_size, segment_size, oom_handling_mem_size);
  }

  auto init(void* buffer, size_t script_mem_size, size_t segment_size, size_t /*unused*/) noexcept -> void {
    kphp::log::assertion(buffer != nullptr);

    m_segment_size = segment_size;
    m_chunk_pool.init(buffer, script_mem_size, m_segment_size + memory_resource::segmented_stack_resource<shared_chunk_pool>::segment_header_size());
  }

  auto free() noexcept -> void {
    auto* curr_buffer{m_chunk_pool.get_buffer_list_head()};
    while (curr_buffer != nullptr) {
      auto* next_buffer = curr_buffer->next_in_chain;
      kphp::memory::platform::free(curr_buffer);
      curr_buffer = next_buffer;
    }
  }

  auto init_resource(memory_resource::segmented_stack_resource<shared_chunk_pool>* resource) const noexcept -> void {
    kphp::log::assertion(resource != nullptr);

    // we can pass nullptr as buffer and 0 as buffer_size, because segment pool is already initialized
    resource->init(nullptr, 0, m_segment_size);
  }

  auto current() const noexcept -> memory_resource::segmented_stack_resource<shared_chunk_pool>* {
    return m_curr_resource;
  }

  auto segment_size() const noexcept -> size_t {
    return m_segment_size;
  }

  auto set(memory_resource::segmented_stack_resource<shared_chunk_pool>* resource) noexcept -> void {
    m_curr_resource = resource;
  }

  auto
  exchange(memory_resource::segmented_stack_resource<shared_chunk_pool>* resource) noexcept -> memory_resource::segmented_stack_resource<shared_chunk_pool>* {
    auto* prev_resource{m_curr_resource};
    set(resource);

    return prev_resource;
  }

  auto alloc_script_memory(size_t size) noexcept -> void* {
    kphp::log::assertion(size != 0);
    kphp::log::assertion(m_curr_resource != nullptr);

    void* mem{m_curr_resource->allocate(size)};
    if (mem == nullptr) [[unlikely]] {
      request_extra_memory(size);
      mem = m_curr_resource->allocate(size);

      kphp::log::assertion(mem != nullptr);
    }

    return mem;
  }

  auto calloc_script_memory(size_t size) noexcept -> void* {
    kphp::log::assertion(size != 0);
    kphp::log::assertion(m_curr_resource != nullptr);

    void* mem{m_curr_resource->allocate0(size)};
    if (mem == nullptr) [[unlikely]] {
      request_extra_memory(size);
      mem = m_curr_resource->allocate0(size);

      kphp::log::assertion(mem != nullptr);
    }

    return mem;
  }

  auto free_script_memory(void* mem, size_t size) noexcept -> void {
    kphp::log::assertion(size != 0);

    m_curr_resource->deallocate(mem, size);
  }
};

} // namespace kphp::coro::detail::memory
