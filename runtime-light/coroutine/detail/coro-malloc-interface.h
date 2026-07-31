// Compiler for PHP (aka KPHP)
// Copyright (c) 2025 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <limits>

#include "common/wrappers/likely.h"
#include "runtime-common/core/allocator/runtime-allocator.h"
#include "runtime-common/core/utils/kphp-assert-core.h"
#include "runtime-light/coroutine/coroutine-state.h"

namespace kphp {

namespace coro {

namespace details {

constexpr uint64_t MALLOC_REPLACER_MAX_ALLOC = 0xFFFFFF00; // 4GiB

struct control_block {
private:
  static constexpr auto SIZE_FIELD_BITSIZE{48};
  static constexpr auto BASE_OFFSET_FIELD_BITSIZE{16};
  static constexpr uint64_t BLOCK_SIZE_MASK{(1UL << SIZE_FIELD_BITSIZE) - 1};
  static constexpr uint64_t BASE_OFFSET_MASK{(1UL << BASE_OFFSET_FIELD_BITSIZE) - 1};

  static_assert(SIZE_FIELD_BITSIZE + BASE_OFFSET_FIELD_BITSIZE == std::numeric_limits<uint64_t>::digits);

public:
  static constexpr uint64_t max_size() noexcept {
    return 1UL << SIZE_FIELD_BITSIZE;
  }

  static constexpr uint64_t max_alignment() noexcept {
    return 1UL << BASE_OFFSET_FIELD_BITSIZE;
  }

  uint64_t raw() const noexcept {
    return (static_cast<uint64_t>(base_offset) << SIZE_FIELD_BITSIZE) | (static_cast<uint64_t>(size) & BLOCK_SIZE_MASK);
  }

  static control_block from_raw(uint64_t raw) noexcept {
    return control_block{.size = raw & BLOCK_SIZE_MASK, .base_offset = static_cast<uint16_t>((raw >> SIZE_FIELD_BITSIZE) & BASE_OFFSET_MASK)};
  }

  uint64_t size : SIZE_FIELD_BITSIZE;
  uint16_t base_offset : BASE_OFFSET_FIELD_BITSIZE;
};

inline bool is_power_of_2(uint64_t v) noexcept {
  return v && !(v & (v - 1));
}

static_assert(sizeof(control_block) == sizeof(uint64_t), "Control block's size must be equal to uint64");

} // namespace details

inline void* alloc(size_t size) noexcept {
  constexpr size_t cb_size{sizeof(details::control_block)};
  if (unlikely(size > std::min(details::control_block::max_size(), kphp::coro::details::MALLOC_REPLACER_MAX_ALLOC) - cb_size)) {
    php_warning("attempt to allocate too much memory by malloc replacer, requested : %lu", size);
    return nullptr;
  }
  const size_t total_size{size + cb_size};
  void* base{CoroutineInstanceState::get().coro_allocator.alloc_script_memory(total_size)};
  if (unlikely(base == nullptr)) {
    php_warning("not enough script memory to allocate, requested : %lu, actual requested: %lu", size, total_size);
    return base;
  }
  *(static_cast<uint64_t*>(base)) = details::control_block{.size = total_size, .base_offset = cb_size}.raw();
  return static_cast<void*>(static_cast<uint8_t*>(base) + cb_size);
}

inline void* alloc_aligned(size_t size, std::align_val_t alignment) noexcept {
  // Check that provided alignment is power of two
  const size_t align{static_cast<uint64_t>(alignment)};
  if (unlikely(align == 0 || !details::is_power_of_2(align) || align >= details::control_block::max_alignment())) {
    php_warning("allocation alignment have to be non-zero power of two and not greater than %" PRIu64 ", got : %lu", details::control_block::max_alignment(),
                align);
    return nullptr;
  }

  // Check that memory is enough
  constexpr size_t cb_size{sizeof(details::control_block)};
  if (unlikely(size > std::min(details::control_block::max_size(), kphp::coro::details::MALLOC_REPLACER_MAX_ALLOC) - (align - 1) - cb_size)) {
    php_warning("attempt to allocate too much memory by malloc replacer, requested : %lu", size);
    return nullptr;
  }

  // Request mem from underlying memory manager
  const size_t total_size{size + (align - 1) + cb_size};
  void* base{CoroutineInstanceState::get().coro_allocator.alloc_script_memory(total_size)};
  if (unlikely(base == nullptr)) {
    php_warning("not enough script memory to allocate, requested : %lu, actual requested: %lu", size, total_size);
    return base;
  }

  const uint64_t base_u{reinterpret_cast<uint64_t>(base)};
  // The smallest multiple of `align` greater than or equal to requested memory
  const uint64_t aligned_u{((base_u + cb_size) + (align - 1)) & ~(align - 1)};
  const uint64_t base_offset_u{aligned_u - base_u};

  // Save control block
  *(reinterpret_cast<uint64_t*>(aligned_u - cb_size)) = // NOLINT
      details::control_block{.size = total_size, .base_offset = static_cast<std::uint16_t>(base_offset_u)}.raw();

  return reinterpret_cast<void*>(aligned_u); // NOLINT
}

inline void* calloc(size_t num, size_t size) noexcept {
  void* ptr{kphp::coro::alloc(num * size)};
  if (unlikely(ptr == nullptr)) {
    return nullptr;
  }
  return std::memset(ptr, 0, num * size);
}

inline void free(void* ptr) noexcept {
  if (unlikely(ptr == nullptr)) {
    return;
  }

  constexpr size_t cb_size{sizeof(details::control_block)};
  const auto mem{reinterpret_cast<uint64_t>(ptr)};

  const auto cb{details::control_block::from_raw(*reinterpret_cast<uint64_t*>(mem - cb_size))}; // NOLINT
  void* base{reinterpret_cast<void*>(mem - cb.base_offset)};                                    // NOLINT

  CoroutineInstanceState::get().coro_allocator.free_script_memory(base, cb.size);
}

inline void* realloc(void* ptr, size_t new_size) noexcept {
  if (unlikely(ptr == nullptr)) {
    return kphp::coro::alloc(new_size);
  }

  if (unlikely(new_size == 0)) {
    kphp::coro::free(ptr);
    return nullptr;
  }

  constexpr size_t cb_size{sizeof(details::control_block)};
  const auto mem{reinterpret_cast<uint64_t>(ptr)};

  const auto cb{details::control_block::from_raw(*reinterpret_cast<uint64_t*>(mem - cb_size))}; // NOLINT

  void* old_base{reinterpret_cast<void*>(mem - cb.base_offset)}; // NOLINT
  const size_t old_size{cb.size};

  void* new_ptr{kphp::coro::alloc(new_size)};
  if (likely(new_ptr != nullptr)) {
    std::memcpy(new_ptr, ptr, std::min(new_size, old_size));
    CoroutineInstanceState::get().coro_allocator.free_script_memory(old_base, old_size);
  }
  return new_ptr;
}

inline char* strdup(const char* str1) noexcept {
  auto* str2{static_cast<char*>(kphp::coro::alloc(std::strlen(str1) + 1))};
  return std::strcpy(str2, str1);
}

} // namespace coro

} // namespace kphp
