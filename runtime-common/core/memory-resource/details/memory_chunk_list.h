// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <algorithm>
#include <memory>

#include "runtime-common/core/memory-resource/memory_resource.h"
#include "runtime-common/core/utils/kphp-assert-core.h"

namespace memory_resource {
namespace details {

class memory_chunk_list {
public:
  memory_chunk_list() = default;

  void* get_mem() noexcept {
    void* result = next_;
    if (next_) {
      next_ = next_->next_;
    }
    return result;
  }

  void put_mem(void* block) noexcept {
    next_ = new (block) memory_chunk_list{next_};
  }

private:
  explicit memory_chunk_list(memory_chunk_list* next) noexcept
      : next_(next) {}

  memory_chunk_list* next_{nullptr};
};

static_assert(sizeof(memory_chunk_list) == 8, "sizeof memory_chunk_list should be 8");

inline constexpr size_t align_for_chunk(size_t size) noexcept {
  constexpr size_t align{8};
  return (size + (align - 1)) & ~(align - 1);
}

inline constexpr size_t align_for_chunk(size_t size, size_t align) noexcept {
  php_assert(align != 0 && (align & (align - 1)) == 0); // NOLINT
  // we need to carve out X bytes such that size + (ptr % align) <= X holds for any ptr the pool may return.
  // the pool only guarantees 8-byte aligned ptr, so the worst case is ptr % align == align - 8.
  // requesting size + (align - 8), rounded up to the pool's 8-byte granularity, is therefore always enough.
  const size_t padding{std::max(align, size_t{8}) - 8};
  return align_for_chunk(size + padding);
}

inline constexpr size_t get_chunk_id(size_t aligned_size) noexcept {
  return aligned_size >> 3;
}

inline constexpr size_t get_chunk_size(size_t chunk_id) noexcept {
  return chunk_id << 3;
}

} // namespace details
} // namespace memory_resource
