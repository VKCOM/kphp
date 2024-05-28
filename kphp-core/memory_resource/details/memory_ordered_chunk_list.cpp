// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "kphp-core/memory_resource/details/memory_ordered_chunk_list.h"

#include <algorithm>
#include <functional>

namespace memory_resource {
namespace details {

memory_ordered_chunk_list::memory_ordered_chunk_list(char *memory_resource_begin) noexcept:
  memory_resource_begin_(memory_resource_begin) {
  static_assert(sizeof(list_node) == 8, "8 bytes expected");
}

memory_ordered_chunk_list::list_node *memory_ordered_chunk_list::flush() noexcept {
  add_from_array(tmp_buffer_.data(), tmp_buffer_.data() + tmp_buffer_size_);
  tmp_buffer_size_ = 0;

  memory_ordered_chunk_list::list_node *ret = head_;
  head_ = nullptr;
  return ret;
}

void memory_ordered_chunk_list::save_next(list_node *node, const list_node *next) const noexcept {
  node->next_chunk_offset_ = static_cast<uint32_t>(reinterpret_cast<const char *>(next) - memory_resource_begin_);
}

void memory_ordered_chunk_list::add_from_array(list_node **first, list_node **last) noexcept {
  if (first == last) {
    return;
  }

  std::sort(first, last, std::greater<>{});
  if (!head_) {
    head_ = *first++;
  } else if (reinterpret_cast<char *>(head_) < reinterpret_cast<char *>(*first)) {
    list_node *next = head_;
    head_ = *first++;
    save_next(head_, next);
  }

  // current is always greater than *first
  list_node *current = head_;
  for (; first != last && current->has_next();) {
    list_node *next = get_next(current);
    if (reinterpret_cast<char *>(*first) < reinterpret_cast<char *>(next)) {
      current = next;
    } else {
      (*first)->next_chunk_offset_ = current->next_chunk_offset_;
      save_next(current, *first);
      current = *first++;
    }
  }

  // current->next_chunk_offset_ is zero
  for (; first != last;) {
    save_next(current, *first);
    current = *first++;
  }

  // merge adjacent nodes
  for (list_node *next = get_next(head_); next; next = get_next(head_)) {
    if (reinterpret_cast<char *>(head_) == reinterpret_cast<char *>(next) + next->chunk_size_) {
      next->chunk_size_ += (head_)->chunk_size_;
      head_ = next;
    } else {
      break;
    }
  }

  list_node *prev = head_;
  if (list_node *node = get_next(prev)) {
    for (list_node *next = get_next(node); next; next = get_next(node)) {
      if (reinterpret_cast<char *>(node) == reinterpret_cast<char *>(next) + next->chunk_size_) {
        next->chunk_size_ += node->chunk_size_;
      } else {
        save_next(prev, node);
        prev = node;
      }
      node = next;
    }
    save_next(prev, node);
  }
}

} // namespace details
} //namespace memory_resource
