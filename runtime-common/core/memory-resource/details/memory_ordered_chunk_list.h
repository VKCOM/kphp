// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <array>
#include <limits>
#include <new>

#include "common/mixin/not_copyable.h"

#include "runtime-common/core/memory-resource/memory_resource.h"
#include "runtime-common/core/utils/kphp-assert-core.h"

namespace memory_resource {
namespace details {

class memory_ordered_chunk_list : vk::not_copyable {
public:
  class list_node {
  public:
    friend class memory_ordered_chunk_list;

    explicit list_node(uint32_t size = 0) noexcept :
      chunk_size_(size) {
    }

    uint32_t size() const noexcept { return chunk_size_; }
    bool has_next() const noexcept { return next_chunk_offset_ != std::numeric_limits<uint32_t>::max(); }

  private:
    uint32_t next_chunk_offset_{std::numeric_limits<uint32_t>::max()};
    uint32_t chunk_size_{0};
  };

  explicit memory_ordered_chunk_list(char *memory_resource_begin, char *memory_resource_end) noexcept;

  list_node *get_next(const list_node *node) const noexcept {
    return node->has_next() ? reinterpret_cast<list_node *>(memory_resource_begin_ + node->next_chunk_offset_) : nullptr;
  }

  void add_memory(void *mem, size_t size) noexcept {
    php_assert(size >= sizeof(list_node) && size <= std::numeric_limits<uint32_t>::max());
    tmp_buffer_[tmp_buffer_size_++] = new(mem) list_node{static_cast<uint32_t>(size)};
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
  char *memory_resource_end_{nullptr};
  list_node *head_{nullptr};
  size_t tmp_buffer_size_{0};
  std::array<list_node *, 4096> tmp_buffer_;
};

} // namespace details
} // namespace memory_resource
