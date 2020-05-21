#pragma once

#include <memory>

#include "common/allocators/freelist.h"

#include "runtime/memory_resource/memory_resource.h"

namespace memory_resource {
namespace details {

class memory_chunk_list {
public:
  memory_chunk_list() = default;

  void *get_mem() noexcept {
    void *result = next_;
    if (next_) {
      next_ = next_->next_;
    }
    return result;
  }

  void put_mem(void *block) noexcept {
    next_ = new(block) memory_chunk_list{next_};
  }

private:
  explicit memory_chunk_list(memory_chunk_list *next) noexcept :
    next_(next) {
  }

  memory_chunk_list *next_{nullptr};
};

static_assert(sizeof(memory_chunk_list) == 8, "sizeof memory_chunk_list should be 8");

inline constexpr size_type align_for_chunk(size_type size) noexcept {
  return (size + 7) & -8;
}

inline constexpr size_type get_chunk_id(size_type aligned_size) noexcept {
  return aligned_size >> 3;
}

inline constexpr size_type get_chunk_size(size_type chunk_id) noexcept {
  return chunk_id << 3;
}

} // namespace details
} // namespace memory_resource
