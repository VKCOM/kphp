#pragma once

#include <array>
#include <limits>
#include <new>

#include "common/mixin/not_copyable.h"

#include "runtime/memory_resource/memory_resource.h"
#include "runtime/php_assert.h"

namespace memory_resource {
namespace details {

class memory_ordered_chunk_list : vk::not_copyable {
public:
  class list_node {
  public:
    friend class memory_ordered_chunk_list;

    explicit list_node(size_type size = 0) noexcept :
      chunk_size_(size) {
    }

    size_type size() const noexcept { return chunk_size_; }
    bool has_next() const noexcept { return next_chunk_offset_ != std::numeric_limits<size_type>::max(); }

  private:
    size_type next_chunk_offset_{std::numeric_limits<size_type>::max()};
    size_type chunk_size_{0};
  };

  explicit memory_ordered_chunk_list(char *memory_resource_begin) noexcept;

  list_node *get_next(const list_node *node) const noexcept {
    return node->has_next() ? reinterpret_cast<list_node *>(memory_resource_begin_ + node->next_chunk_offset_) : nullptr;
  }

  void add_memory(void *mem, size_type size) noexcept {
    php_assert(size >= sizeof(list_node));
    tmp_buffer_[tmp_buffer_size_++] = new(mem) list_node{size};
    if (tmp_buffer_size_ == tmp_buffer_.size()) {
      add_from_array(tmp_buffer_.data(), tmp_buffer_.data() + tmp_buffer_size_);
      tmp_buffer_size_ = 0;
    }
  }

  list_node *flush() noexcept;

private:
  void save_next(list_node *node, const list_node *next) const noexcept;
  void add_from_array(list_node **first, list_node **last) noexcept;

  char *memory_resource_begin_{nullptr};
  list_node *head_{nullptr};
  size_t tmp_buffer_size_{0};
  std::array<list_node *, 4096> tmp_buffer_;
};

} // namespace details
} // namespace memory_resource
