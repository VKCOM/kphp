#include "runtime/memory_resource/details/memory_ordered_chunk_list.h"

#include <algorithm>
#include <functional>

namespace memory_resource {
namespace details {

memory_chunk_list_node *memory_ordered_chunk_list::flush() noexcept {
  add_from_array(tmp_buffer_.data(), tmp_buffer_.data() + tmp_buffer_size_);
  tmp_buffer_size_ = 0;

  memory_chunk_list_node *ret = head_;
  head_ = nullptr;
  return ret;
}

void memory_ordered_chunk_list::add_from_array(memory_chunk_list_node **first, memory_chunk_list_node **last) noexcept {
  if (first == last) {
    return;
  }

  std::sort(first, last, std::greater<>{});
  if (!head_) {
    head_ = *first++;
  } else if (reinterpret_cast<char *>(head_) < reinterpret_cast<char *>(*first)) {
    memory_chunk_list_node *next = head_;
    head_ = *first++;
    head_->set_next(next);
  }

  // current is always greater than *first
  memory_chunk_list_node *current = head_;
  for (; first != last && current->get_next();) {
    memory_chunk_list_node *next = current->get_next();
    if (reinterpret_cast<char *>(*first) < reinterpret_cast<char *>(next)) {
      current = next;
    } else {
      (*first)->set_next(current->get_next());
      current->set_next(*first);
      current = *first++;
    }
  }

  // current->next_chunk_offset_ is zero
  for (; first != last;) {
    current->set_next(*first);
    current = *first++;
  }

  // merge adjacent nodes
  for (memory_chunk_list_node *next = head_->get_next(); next; next = head_->get_next()) {
    if (reinterpret_cast<char *>(head_) == reinterpret_cast<char *>(next) + next->size()) {
      next->extend_size(head_->size());
      head_ = next;
    } else {
      break;
    }
  }

  memory_chunk_list_node *prev = head_;
  if (memory_chunk_list_node *node = prev->get_next()) {
    for (memory_chunk_list_node *next = node->get_next(); next; next = node->get_next()) {
      if (reinterpret_cast<char *>(node) == reinterpret_cast<char *>(next) + next->size()) {
        next->extend_size(node->size());
      } else {
        prev->set_next(node);
        prev = node;
      }
      node = next;
    }
    prev->set_next(node);
  }
}

} // namespace details
} //namespace memory_resource
