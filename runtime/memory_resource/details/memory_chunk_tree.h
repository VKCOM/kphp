#pragma once

#include "runtime/memory_resource/memory_resource.h"

namespace memory_resource {
namespace details {

class memory_chunk_node;

class memory_chunk_tree {
public:
  void insert(void *mem, size_type size);
  memory_chunk_node *extract(size_type size);

  static size_type get_chunk_size(memory_chunk_node *node);

private:
  memory_chunk_node *search(size_type size, bool lower_bound);

  void left_rotate(memory_chunk_node *node);
  void right_rotate(memory_chunk_node *node);
  void fix_red_red(memory_chunk_node *node);
  memory_chunk_node *find_replacer(memory_chunk_node *node);
  void detach_leaf(memory_chunk_node *detaching_node);
  void detach_node_with_one_child(memory_chunk_node *detaching_node, memory_chunk_node *replacer);
  void swap_detaching_node_with_replacer(memory_chunk_node *detaching_node, memory_chunk_node *replacer);
  void detach_node(memory_chunk_node *detaching_node);
  void fix_double_black(memory_chunk_node *node);

  memory_chunk_node *root_{nullptr};
};

} // namespace details
} // namespace memory_resource
