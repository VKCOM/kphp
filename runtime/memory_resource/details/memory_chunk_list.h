#pragma once

#include <memory>

#include "runtime/memory_resource/memory_resource.h"

namespace memory_resource {
namespace details {

class memory_chunk_list_node {
public:
  explicit memory_chunk_list_node(memory_chunk_list_node *next = nullptr, size_t size = 0) noexcept :
    next_(next),
    size_(size) {
  }

  size_t size() const noexcept { return size_; }
  memory_chunk_list_node *get_next() const noexcept { return next_; }

  void extend_size(size_t size) noexcept { size_ += size; }
  void set_next(memory_chunk_list_node *next) noexcept { next_ = next; }

private:
  memory_chunk_list_node *next_{nullptr};
  size_t size_{0};
};

class memory_chunk_list {
public:
  void *get_mem() noexcept {
    memory_chunk_list_node *next = head_.get_next();
    if (next) {
      head_.set_next(next->get_next());
    }
    return next;
  }

  void put_mem(void *block) noexcept {
    head_.set_next(new(block) memory_chunk_list_node{head_.get_next()});
  }

private:
  memory_chunk_list_node head_;
};

static_assert(sizeof(memory_chunk_list_node) == 16, "sizeof memory_chunk_list should be 16");

inline size_t align_for_chunk(size_t size) noexcept {
  // минимальный размер куска - 16 байт
  return std::max(static_cast<size_t>((size + 7L) & -8L), 16UL);
}

inline constexpr size_t get_chunk_id(size_t aligned_size) noexcept {
  return aligned_size >> 3;
}

inline constexpr size_t get_chunk_size(size_t chunk_id) noexcept {
  return chunk_id << 3;
}

} // namespace details
} // namespace memory_resource
