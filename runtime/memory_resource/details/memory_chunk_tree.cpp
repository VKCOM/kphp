#include "runtime/memory_resource/details/memory_chunk_tree.h"

#include <new>
#include <utility>

#include "runtime/memory_resource/memory_resource.h"
#include "runtime/php_assert.h"

namespace memory_resource {
namespace details {

class memory_chunk_node {
public:
  memory_chunk_node *left{nullptr};
  memory_chunk_node *right{nullptr};
  memory_chunk_node *parent{nullptr};
  enum {
    RED,
    BLACK
  } color{RED};
  size_type chunk_size;
  memory_chunk_node *same_size_chunk_list{nullptr};

  explicit memory_chunk_node(size_type size) :
    chunk_size(size) {
  }

  memory_chunk_node *uncle() const {
    if (!parent || !parent->parent) {
      return nullptr;
    }
    auto grandpa = parent->parent;
    return parent->is_left() ? grandpa->right : grandpa->left;
  }

  bool is_left() const {
    return this == parent->left;
  }

  void replace_self_on_parent(memory_chunk_node *replacer) {
    if (is_left()) {
      parent->left = replacer;
    } else {
      parent->right = replacer;
    }
  }

  memory_chunk_node *sibling() const {
    if (!parent) {
      return nullptr;
    }
    return is_left() ? parent->right : parent->left;
  }

  void move_down(memory_chunk_node *new_parent) {
    if (parent) {
      replace_self_on_parent(new_parent);
    }
    new_parent->parent = parent;
    parent = new_parent;
  }

  bool has_red_child() const {
    return (left && left->color == RED) || (right && right->color == RED);
  }
};

void memory_chunk_tree::insert(void *mem, size_type size) {
  php_assert(sizeof(memory_chunk_node) <= size);
  memory_chunk_node *newNode = new(mem) memory_chunk_node{size};
  if (!root_) {
    newNode->color = memory_chunk_node::BLACK;
    root_ = newNode;
  } else {
    memory_chunk_node *temp = search(size, false);
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

memory_chunk_node *memory_chunk_tree::extract(size_type size) {
  if (memory_chunk_node *v = search(size, true)) {
    if (v->same_size_chunk_list) {
      memory_chunk_node *result = v->same_size_chunk_list;
      v->same_size_chunk_list = result->same_size_chunk_list;
      return result;
    }
    detach_node(v);
    return v;
  }
  return nullptr;
}

size_type memory_chunk_tree::get_chunk_size(memory_chunk_node *node) {
  return node->chunk_size;
}

memory_chunk_node *memory_chunk_tree::search(size_type size, bool lower_bound) {
  memory_chunk_node *node = root_;
  memory_chunk_node *lower_bound_node = nullptr;
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

void memory_chunk_tree::left_rotate(memory_chunk_node *node) {
  memory_chunk_node *new_parent = node->right;
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

void memory_chunk_tree::right_rotate(memory_chunk_node *node) {
  memory_chunk_node *new_parent = node->left;
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

void memory_chunk_tree::fix_red_red(memory_chunk_node *node) {
  if (node == root_) {
    node->color = memory_chunk_node::BLACK;
    return;
  }

  memory_chunk_node *parent = node->parent;
  memory_chunk_node *grandparent = parent->parent;
  memory_chunk_node *uncle = node->uncle();

  if (parent->color != memory_chunk_node::BLACK) {
    if (uncle && uncle->color == memory_chunk_node::RED) {
      parent->color = memory_chunk_node::BLACK;
      uncle->color = memory_chunk_node::BLACK;
      grandparent->color = memory_chunk_node::RED;
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

memory_chunk_node *memory_chunk_tree::find_replacer(memory_chunk_node *node) {
  if (node->left && node->right) {
    // find node that do not have a left child
    memory_chunk_node *replacer = node->right;
    while (replacer->left) {
      replacer = replacer->left;
    }
    return replacer;
  }
  return node->left ? node->left : node->right;
}

void memory_chunk_tree::detach_leaf(memory_chunk_node *detaching_node) {
  if (detaching_node == root_) {
    root_ = nullptr;
    return;
  }

  if (detaching_node->color == memory_chunk_node::BLACK) {
    fix_double_black(detaching_node);
  } else {
    // replacer or detaching_node is red
    if (memory_chunk_node *sibling = detaching_node->sibling()) {
      sibling->color = memory_chunk_node::RED;
    }
  }

  detaching_node->replace_self_on_parent(nullptr);
}

void memory_chunk_tree::detach_node_with_one_child(memory_chunk_node *detaching_node, memory_chunk_node *replacer) {
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
  if (replacer->color == memory_chunk_node::BLACK && detaching_node->color == memory_chunk_node::BLACK) {
    fix_double_black(replacer);
  } else {
    // replacer or detaching_node red
    replacer->color = memory_chunk_node::BLACK;
  }
}

void memory_chunk_tree::swap_detaching_node_with_replacer(memory_chunk_node *detaching_node, memory_chunk_node *replacer) {
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

void memory_chunk_tree::detach_node(memory_chunk_node *detaching_node) {
  memory_chunk_node *replacer = find_replacer(detaching_node);
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

void memory_chunk_tree::fix_double_black(memory_chunk_node *node) {
  if (node == root_) {
    return;
  }

  memory_chunk_node *parent = node->parent;
  if (memory_chunk_node *sibling = node->sibling()) {
    if (sibling->color == memory_chunk_node::RED) {
      parent->color = memory_chunk_node::RED;
      sibling->color = memory_chunk_node::BLACK;
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
      if (sibling->left && sibling->left->color == memory_chunk_node::RED) {
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
      parent->color = memory_chunk_node::BLACK;
      return;
    }

    // 2 black children
    sibling->color = memory_chunk_node::RED;
    if (parent->color == memory_chunk_node::BLACK) {
      fix_double_black(parent);
    } else {
      parent->color = memory_chunk_node::BLACK;
    }
    return;
  }

  // no sibling, double black pushed up
  fix_double_black(parent);
}

} // namespace details
} // namespace memory_resource
