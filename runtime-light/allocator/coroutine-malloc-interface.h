// Compiler for PHP (aka KPHP)
// Copyright (c) 2026 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>

#include "common/wrappers/likely.h"
#include "runtime-common/core/allocator/detail/control-block.h"
#include "runtime-common/core/utils/kphp-assert-core.h"
#include "runtime-light/allocator/runtime-coroutine-allocator.h"

namespace kphp::memory::coro {

constexpr uint64_t MALLOC_REPLACER_MAX_ALLOC = 0xFFFFFF00; // 4GiB

inline void* alloc(size_t size) noexcept {
  constexpr size_t cb_size{sizeof(kphp::memory::detail::control_block)};
  if (unlikely(size > std::min(kphp::memory::detail::control_block::max_size(), MALLOC_REPLACER_MAX_ALLOC) - cb_size)) {
    php_warning("attempt to allocate too much memory by malloc replacer, requested : %lu", size);
    return nullptr;
  }
  const size_t total_size{size + cb_size};
  void* base{RuntimeCoroutineAllocator::get().alloc_memory(total_size)};
  if (unlikely(base == nullptr)) {
    php_warning("not enough coroutine memory to allocate, requested : %lu, actual requested: %lu", size, total_size);
    return base;
  }
  *(static_cast<uint64_t*>(base)) = kphp::memory::detail::control_block{.size = total_size, .base_offset = cb_size}.raw();
  return static_cast<void*>(static_cast<uint8_t*>(base) + cb_size);
}

inline void* alloc_aligned(size_t size, std::align_val_t alignment) noexcept {
  // Check that provided alignment is power of two
  const size_t align{static_cast<uint64_t>(alignment)};
  if (unlikely(align == 0 || !kphp::memory::detail::is_power_of_2(align) || align >= kphp::memory::detail::control_block::max_alignment())) {
    php_warning("allocation alignment have to be non-zero power of two and not greater than %" PRIu64 ", got : %lu",
                kphp::memory::detail::control_block::max_alignment(), align);
    return nullptr;
  }

  // Check that memory is enough
  constexpr size_t cb_size{sizeof(kphp::memory::detail::control_block)};
  if (unlikely(size > std::min(kphp::memory::detail::control_block::max_size(), MALLOC_REPLACER_MAX_ALLOC) - (align - 1) - cb_size)) {
    php_warning("attempt to allocate too much memory by malloc replacer, requested : %lu", size);
    return nullptr;
  }

  // Request mem from underlying memory manager
  const size_t total_size{size + (align - 1) + cb_size};
  void* base{RuntimeCoroutineAllocator::get().alloc_memory(total_size)};
  if (unlikely(base == nullptr)) {
    php_warning("not enough coroutine memory to allocate, requested : %lu, actual requested: %lu", size, total_size);
    return base;
  }

  const uint64_t base_u{reinterpret_cast<uint64_t>(base)};
  // The smallest multiple of `align` greater than or equal to requested memory
  const uint64_t aligned_u{((base_u + cb_size) + (align - 1)) & ~(align - 1)};
  const uint64_t base_offset_u{aligned_u - base_u};

  // Save control block
  *(reinterpret_cast<uint64_t*>(aligned_u - cb_size)) = // NOLINT
      kphp::memory::detail::control_block{.size = total_size, .base_offset = static_cast<std::uint16_t>(base_offset_u)}.raw();

  return reinterpret_cast<void*>(aligned_u); // NOLINT
}

inline void* calloc(size_t num, size_t size) noexcept {
  void* ptr{kphp::memory::coro::alloc(num * size)};
  if (unlikely(ptr == nullptr)) {
    return nullptr;
  }
  return std::memset(ptr, 0, num * size);
}

inline void free(void* ptr) noexcept {
  if (unlikely(ptr == nullptr)) {
    return;
  }

  constexpr size_t cb_size{sizeof(kphp::memory::detail::control_block)};
  const auto mem{reinterpret_cast<uint64_t>(ptr)};

  const auto cb{kphp::memory::detail::control_block::from_raw(*reinterpret_cast<uint64_t*>(mem - cb_size))}; // NOLINT
  void* base{reinterpret_cast<void*>(mem - cb.base_offset)};                                                 // NOLINT

  RuntimeCoroutineAllocator::get().free_memory(base, cb.size);
}

inline void* realloc(void* ptr, size_t new_size) noexcept {
  if (unlikely(ptr == nullptr)) {
    return kphp::memory::coro::alloc(new_size);
  }

  if (unlikely(new_size == 0)) {
    kphp::memory::coro::free(ptr);
    return nullptr;
  }

  constexpr size_t cb_size{sizeof(kphp::memory::detail::control_block)};
  const auto mem{reinterpret_cast<uint64_t>(ptr)};

  const auto cb{kphp::memory::detail::control_block::from_raw(*reinterpret_cast<uint64_t*>(mem - cb_size))}; // NOLINT

  void* old_base{reinterpret_cast<void*>(mem - cb.base_offset)}; // NOLINT
  const size_t old_size{cb.size};

  void* new_ptr{kphp::memory::coro::alloc(new_size)};
  if (likely(new_ptr != nullptr)) {
    std::memcpy(new_ptr, ptr, std::min(new_size, old_size));
    RuntimeCoroutineAllocator::get().free_memory(old_base, old_size);
  }
  return new_ptr;
}

} // namespace kphp::memory::coro
