// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime/memory_resource/details/memory_chunk_tree.h"

#include <new>
#include <utility>

#include "runtime/memory_resource/details/memory_ordered_chunk_list.h"
#include "runtime/php_assert.h"

namespace memory_resource {
namespace details {

class memory_chunk_tree::tree_node {
public:
  tree_node *left{nullptr};
  tree_node *right{nullptr};
  tree_node *parent{nullptr};
  enum { RED, BLACK } color{RED};
  size_t chunk_size;
  tree_node *same_size_chunk_list{nullptr};

  explicit tree_node(size_t size) noexcept
    : chunk_size(size) {}

  tree_node *uncle() const noexcept {
    if (!parent || !parent->parent) {
      return nullptr;
    }
    auto *grandpa = parent->parent;
    return parent->is_left() ? grandpa->right : grandpa->left;
  }

  bool is_left() const noexcept {
    return this == parent->left;
  }

  void replace_self_on_parent(tree_node *replacer) noexcept {
    if (is_left()) {
      parent->left = replacer;
    } else {
      parent->right = replacer;
    }
  }

  tree_node *sibling() const noexcept {
    if (!parent) {
      return nullptr;
    }
    return is_left() ? parent->right : parent->left;
  }

  void move_down(tree_node *new_parent) noexcept {
    if (parent) {
      replace_self_on_parent(new_parent);
    }
    new_parent->parent = parent;
    parent = new_parent;
  }

  bool has_red_child() const noexcept {
    return (left && left->color == RED) || (right && right->color == RED);
  }
};

void memory_chunk_tree::insert(void *mem, size_t size) noexcept {
  php_assert(sizeof(tree_node) <= size);
  tree_node *newNode = new (mem) tree_node{size};
  if (!root_) {
    newNode->color = tree_node::BLACK;
    root_ = newNode;
  } else {
    tree_node *temp = search(size, false);
    if (temp->chunk_size == size) {
      newNode->same_size_chunk_list = temp->same_size_chunk_list;
      temp->same_size_chunk_list = newNode;
      return;
    }

    newNode->parent = temp;
    if (size < temp->chunk_size) {
      temp->left = newNode;
    } else {
      temp->right = newNode;
    }

    fix_red_red(newNode);
  }
}

memory_chunk_tree::tree_node *memory_chunk_tree::extract(size_t size) noexcept {
  if (tree_node *v = search(size, true)) {
    if (v->same_size_chunk_list) {
      tree_node *result = v->same_size_chunk_list;
      v->same_size_chunk_list = result->same_size_chunk_list;
      return result;
    }
    detach_node(v);
    return v;
  }
  return nullptr;
}

memory_chunk_tree::tree_node *memory_chunk_tree::extract_smallest() noexcept {
  tree_node *v = root_;
  while (v && v->left) {
    v = v->left;
  }
  if (v) {
    if (v->same_size_chunk_list) {
      tree_node *result = v->same_size_chunk_list;
      v->same_size_chunk_list = result->same_size_chunk_list;
      return result;
    }
    detach_node(v);
  }
  return v;
}

bool memory_chunk_tree::has_memory_for(size_t size) const noexcept {
  return search(size, true);
}

size_t memory_chunk_tree::get_chunk_size(tree_node *node) noexcept {
  return node->chunk_size;
}

void memory_chunk_tree::flush_to(memory_ordered_chunk_list &mem_list) noexcept {
  flush_node_to(root_, mem_list);
  root_ = nullptr;
}

void memory_chunk_tree::flush_node_to(tree_node *node, memory_ordered_chunk_list &mem_list) noexcept {
  if (!node) {
    return;
  }

  while (node->same_size_chunk_list) {
    tree_node *next = node->same_size_chunk_list;
    node->same_size_chunk_list = next->same_size_chunk_list;
    mem_list.add_memory(next, next->chunk_size);
  }
  tree_node *left = node->left;
  tree_node *right = node->right;
  mem_list.add_memory(node, node->chunk_size);

  flush_node_to(left, mem_list);
  flush_node_to(right, mem_list);
}

memory_chunk_tree::tree_node *memory_chunk_tree::search(size_t size, bool lower_bound) const noexcept {
  tree_node *node = root_;
  tree_node *lower_bound_node = nullptr;
  while (node && size != node->chunk_size) {
    if (size < node->chunk_size) {
      lower_bound_node = node;
      if (!node->left) {
        break;
      } else {
        node = node->left;
      }
    } else {
      if (!node->right) {
        break;
      } else {
        node = node->right;
      }
    }
  }
  if (node && node->chunk_size == size) {
    lower_bound_node = node;
  }
  return lower_bound ? lower_bound_node : node;
}

void memory_chunk_tree::left_rotate(tree_node *node) noexcept {
  tree_node *new_parent = node->right;
  if (node == root_) {
    root_ = new_parent;
  }

  node->move_down(new_parent);
  node->right = new_parent->left;
  if (new_parent->left) {
    new_parent->left->parent = node;
  }

  new_parent->left = node;
}

void memory_chunk_tree::right_rotate(tree_node *node) noexcept {
  tree_node *new_parent = node->left;
  if (node == root_) {
    root_ = new_parent;
  }

  node->move_down(new_parent);
  node->left = new_parent->right;
  if (new_parent->right) {
    new_parent->right->parent = node;
  }

  new_parent->right = node;
}

void memory_chunk_tree::fix_red_red(tree_node *node) noexcept {
  if (node == root_) {
    node->color = tree_node::BLACK;
    return;
  }

  tree_node *parent = node->parent;
  tree_node *grandparent = parent->parent;
  tree_node *uncle = node->uncle();

  if (parent->color != tree_node::BLACK) {
    if (uncle && uncle->color == tree_node::RED) {
      parent->color = tree_node::BLACK;
      uncle->color = tree_node::BLACK;
      grandparent->color = tree_node::RED;
      fix_red_red(grandparent);
    } else {
      if (parent->is_left()) {
        if (node->is_left()) {
          std::swap(parent->color, grandparent->color);
        } else {
          left_rotate(parent);
          std::swap(node->color, grandparent->color);
        }
        right_rotate(grandparent);
      } else {
        if (node->is_left()) {
          right_rotate(parent);
          std::swap(node->color, grandparent->color);
        } else {
          std::swap(parent->color, grandparent->color);
        }
        left_rotate(grandparent);
      }
    }
  }
}

memory_chunk_tree::tree_node *memory_chunk_tree::find_replacer(tree_node *node) noexcept {
  if (node->left && node->right) {
    // find node that do not have a left child
    tree_node *replacer = node->right;
    while (replacer->left) {
      replacer = replacer->left;
    }
    return replacer;
  }
  return node->left ? node->left : node->right;
}

void memory_chunk_tree::detach_leaf(tree_node *detaching_node) noexcept {
  if (detaching_node == root_) {
    root_ = nullptr;
    return;
  }

  if (detaching_node->color == tree_node::BLACK) {
    fix_double_black(detaching_node);
  } else {
    // replacer or detaching_node is red
    if (tree_node *sibling = detaching_node->sibling()) {
      sibling->color = tree_node::RED;
    }
  }

  detaching_node->replace_self_on_parent(nullptr);
}

void memory_chunk_tree::detach_node_with_one_child(tree_node *detaching_node, tree_node *replacer) noexcept {
  if (detaching_node == root_) {
    php_assert(!replacer->left && !replacer->right);
    php_assert(replacer->parent == detaching_node);
    replacer->parent = nullptr;
    replacer->color = detaching_node->color;
    root_ = replacer;
    return;
  }
  // detach detaching_node from tree and move replacer up
  detaching_node->replace_self_on_parent(replacer);
  replacer->parent = detaching_node->parent;
  if (replacer->color == tree_node::BLACK && detaching_node->color == tree_node::BLACK) {
    fix_double_black(replacer);
  } else {
    // replacer or detaching_node red
    replacer->color = tree_node::BLACK;
  }
}

void memory_chunk_tree::swap_detaching_node_with_replacer(tree_node *detaching_node, tree_node *replacer) noexcept {
  if (detaching_node->parent) {
    detaching_node->replace_self_on_parent(replacer);
  } else {
    root_ = replacer;
  }

  if (detaching_node->left && detaching_node->left != replacer) {
    detaching_node->left->parent = replacer;
  }
  if (detaching_node->right && detaching_node->right != replacer) {
    detaching_node->right->parent = replacer;
  }

  if (replacer->left) {
    replacer->left->parent = detaching_node;
  }
  if (replacer->right) {
    replacer->right->parent = detaching_node;
  }

  if (replacer->parent == detaching_node) {
    replacer->parent = detaching_node->parent;
    detaching_node->parent = replacer;
  } else {
    php_assert(replacer->parent);
    replacer->replace_self_on_parent(detaching_node);
    std::swap(replacer->parent, detaching_node->parent);
  }
  std::swap(replacer->left, detaching_node->left);
  std::swap(replacer->right, detaching_node->right);
  std::swap(replacer->color, detaching_node->color);
}

void memory_chunk_tree::detach_node(tree_node *detaching_node) noexcept {
  tree_node *replacer = find_replacer(detaching_node);
  if (!replacer) {
    detach_leaf(detaching_node);
    return;
  }

  if (!detaching_node->left || !detaching_node->right) {
    detach_node_with_one_child(detaching_node, replacer);
    return;
  }

  swap_detaching_node_with_replacer(detaching_node, replacer);
  detach_node(detaching_node);
}

void memory_chunk_tree::fix_double_black(tree_node *node) noexcept {
  if (node == root_) {
    return;
  }

  tree_node *parent = node->parent;
  if (tree_node *sibling = node->sibling()) {
    if (sibling->color == tree_node::RED) {
      parent->color = tree_node::RED;
      sibling->color = tree_node::BLACK;
      if (sibling->is_left()) {
        right_rotate(parent);
      } else {
        left_rotate(parent);
      }
      fix_double_black(node);
      return;
    }
    // sibling is black
    if (sibling->has_red_child()) {
      // at least 1 red children
      if (sibling->left && sibling->left->color == tree_node::RED) {
        if (sibling->is_left()) {
          sibling->left->color = sibling->color;
          sibling->color = parent->color;
          right_rotate(parent);
        } else {
          sibling->left->color = parent->color;
          right_rotate(sibling);
          left_rotate(parent);
        }
      } else {
        if (sibling->is_left()) {
          sibling->right->color = parent->color;
          left_rotate(sibling);
          right_rotate(parent);
        } else {
          sibling->right->color = sibling->color;
          sibling->color = parent->color;
          left_rotate(parent);
        }
      }
      parent->color = tree_node::BLACK;
      return;
    }

    // 2 black children
    sibling->color = tree_node::RED;
    if (parent->color == tree_node::BLACK) {
      fix_double_black(parent);
    } else {
      parent->color = tree_node::BLACK;
    }
    return;
  }

  // no sibling, double black pushed up
  fix_double_black(parent);
}

} // namespace details
} // namespace memory_resource
