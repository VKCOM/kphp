#pragma once

#include <array>
#include <limits>
#include <new>

#include "common/mixin/not_copyable.h"

#include "runtime/memory_resource/details/memory_chunk_list.h"
#include "runtime/memory_resource/memory_resource.h"
#include "runtime/php_assert.h"

namespace memory_resource {
namespace details {

class memory_ordered_chunk_list : vk::not_copyable {
public:
  void add_memory(void *mem, size_t size) noexcept {
    php_assert(size >= sizeof(memory_chunk_list_node));
    tmp_buffer_[tmp_buffer_size_++] = new(mem) memory_chunk_list_node{nullptr, size};
    if (tmp_buffer_size_ == tmp_buffer_.size()) {
      add_from_array(tmp_buffer_.data(), tmp_buffer_.data() + tmp_buffer_size_);
      tmp_buffer_size_ = 0;
    }
  }

  memory_chunk_list_node *flush() noexcept;

private:
  void add_from_array(memory_chunk_list_node **first, memory_chunk_list_node **last) noexcept;

  memory_chunk_list_node *head_{nullptr};
  size_t tmp_buffer_size_{0};
  std::array<memory_chunk_list_node *, 4096> tmp_buffer_;
};

} // namespace details
} // namespace memory_resource
