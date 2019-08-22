#pragma once

#include <memory>

#include "common/allocators/freelist.h"

#include "runtime/memory_resource/memory_resource.h"

namespace memory_resource {
namespace details {

class unsynchronized_memory_chunk_list {
public:
  unsynchronized_memory_chunk_list() = default;

  void *get_mem() {
    void *result = next_;
    if (next_) {
      next_ = next_->next_;
    }
    return result;
  }

  void put_mem(void *block) {
    next_ = new(block) unsynchronized_memory_chunk_list{next_};
  }

private:
  explicit unsynchronized_memory_chunk_list(unsynchronized_memory_chunk_list *next) :
    next_(next) {
  }

  unsynchronized_memory_chunk_list *next_{nullptr};
};

static_assert(sizeof(unsynchronized_memory_chunk_list) == 8, "sizeof unsynchronized_memory_chunk_list should be 8");

class synchronized_memory_chunk_list {
public:
  synchronized_memory_chunk_list() { freelist_init(&list_); }

  void *get_mem() { return freelist_get(&list_); }

  void put_mem(void *block) { freelist_put(&list_, block); }

private:
  freelist_t list_;
};

static_assert(sizeof(freelist_t) == 8, "sizeof freelist_t should be 8");

inline constexpr size_type align_for_chunk(size_type size) {
  return (size + 7) & -8;
}

inline constexpr size_type get_chunk_id(size_type aligned_size) {
  return aligned_size >> 3;
}

} // namespace details
} // namespace memory_resource