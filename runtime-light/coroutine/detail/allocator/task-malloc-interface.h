// Compiler for PHP (aka KPHP)
// Copyright (c) 2026 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <bit>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <new>

#include "runtime-light/coroutine/detail/allocator/coroutine-malloc-interface.h"
#include "runtime-light/coroutine/detail/allocator/runtime-coroutine-allocator.h"
#include "runtime-light/coroutine/detail/allocator/task-allocator.h"

namespace kphp::coro::detail::memory::task {

struct control_block {
private:
  static constexpr uint64_t MALLOC_REPLACER_MAX_ALLOC = 0xFFFFFF00; // 4GiB

public:
  enum class backend_type : uint8_t { task_pool, task_pool_owner, coroutine_pool };

  backend_type backend{};
  uint16_t base_offset{};

  static constexpr auto max_size() noexcept -> uint64_t {
    return MALLOC_REPLACER_MAX_ALLOC;
  }

  static constexpr auto max_alignment() noexcept -> uint64_t {
    return std::numeric_limits<uint16_t>::max();
  }
};

inline auto alloc_aligned(size_t size, std::align_val_t al) noexcept -> void* {
  // Check that provided alignment is power of two
  const size_t align{static_cast<uint64_t>(al)};
  if (align == 0 || !std::has_single_bit(align) || align >= kphp::coro::detail::memory::task::control_block::max_alignment()) [[unlikely]] {
    php_warning("allocation alignment have to be non-zero power of two and not greater than %" PRIu64 ", got : %lu",
                kphp::coro::detail::memory::task::control_block::max_alignment(), align);
    return nullptr;
  }

  // Check that memory is enough
  constexpr size_t cb_size{sizeof(kphp::coro::detail::memory::task::control_block)};
  if (size > kphp::coro::detail::memory::task::control_block::max_size() - (align - 1) - cb_size) [[unlikely]] {
    php_warning("attempt to allocate too much memory by malloc replacer, requested : %lu", size);
    return nullptr;
  }

  // Request mem from underlying memory manager
  const size_t total_size{size + (align - 1) + cb_size};
  void* base{nullptr};
  kphp::coro::detail::memory::task::control_block::backend_type backend{};
  auto& task_allocator{kphp::coro::detail::memory::task_allocator::get()};
  if (task_allocator.consume_stack_request()) {
    if (total_size <= kphp::coro::detail::memory::task_allocator::get().segment_size()) [[likely]] {
      if (task_allocator.current() == nullptr) {
        void* mem{kphp::coro::detail::memory::alloc_aligned(
            sizeof(memory_resource::segmented_stack_resource<kphp::coro::detail::memory::task_allocator::shared_chunk_pool>),
            static_cast<std::align_val_t>(alignof(memory_resource::segmented_stack_resource<kphp::coro::detail::memory::task_allocator::shared_chunk_pool>)))};

        kphp::log::assertion(mem != nullptr);

        auto* stack{
            std::construct_at(static_cast<memory_resource::segmented_stack_resource<kphp::coro::detail::memory::task_allocator::shared_chunk_pool>*>(mem))};
        task_allocator.init_resource(stack);
        task_allocator.set(stack);
        backend = kphp::coro::detail::memory::task::control_block::backend_type::task_pool_owner;
      } else {
        backend = kphp::coro::detail::memory::task::control_block::backend_type::task_pool;
      }

      base = task_allocator.alloc_script_memory(total_size);
    } else {
      base = RuntimeCoroutineAllocator::get().alloc_script_memory(total_size);
      backend = kphp::coro::detail::memory::task::control_block::backend_type::coroutine_pool;
    }
  } else {
    base = RuntimeCoroutineAllocator::get().alloc_script_memory(total_size);
    backend = kphp::coro::detail::memory::task::control_block::backend_type::coroutine_pool;
  }

  if (base == nullptr) [[unlikely]] {
    php_warning("not enough script memory to allocate, requested : %lu, actual requested: %lu", size, total_size);
    return base;
  }

  const uint64_t base_u{reinterpret_cast<uint64_t>(base)};
  // The smallest multiple of `align` greater than or equal to requested memory
  const uint64_t aligned_u{((base_u + cb_size) + (align - 1)) & ~(align - 1)};
  const uint64_t base_offset_u{aligned_u - base_u};

  std::construct_at(reinterpret_cast<kphp::coro::detail::memory::task::control_block*>(aligned_u - cb_size), backend, // NOLINT
                    static_cast<uint16_t>(base_offset_u));

  return reinterpret_cast<void*>(aligned_u); // NOLINT
}

inline auto free_aligned(void* ptr, size_t size, std::align_val_t al) noexcept -> void {
  if (ptr == nullptr) [[unlikely]] {
    return;
  }

  const size_t align{static_cast<uint64_t>(al)};
  const size_t cb_size{sizeof(kphp::coro::detail::memory::task::control_block)};
  const size_t total_size{size + (align - 1) + cb_size};
  auto* cb{reinterpret_cast<kphp::coro::detail::memory::task::control_block*>(static_cast<std::byte*>(ptr) - cb_size)};
  auto& task_allocator{kphp::coro::detail::memory::task_allocator::get()};
  if (cb->backend == kphp::coro::detail::memory::task::control_block::backend_type::task_pool) {
    task_allocator.free_script_memory(reinterpret_cast<std::byte*>(ptr) - cb->base_offset, total_size);
  } else if (cb->backend == kphp::coro::detail::memory::task::control_block::backend_type::task_pool_owner) {
    task_allocator.free_script_memory(reinterpret_cast<std::byte*>(ptr) - cb->base_offset, total_size);
    auto* stack{task_allocator.current()};
    task_allocator.set(nullptr);
    std::destroy_at(stack);
    kphp::coro::detail::memory::free(stack);
  } else {
    RuntimeCoroutineAllocator::get().free_script_memory(reinterpret_cast<std::byte*>(ptr) - cb->base_offset, total_size);
  }
}

} // namespace kphp::coro::detail::memory::task
