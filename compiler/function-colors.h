// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <array>
#include <map>
#include <vector>

#include <common/termformat/termformat.h>
#include <compiler/stage.h>

// function_palette namespace contains all the necessary things to
// implement the concept of colors for functions.
namespace function_palette {

// color_t enumeration describes all possible colors.
enum class color_t {
  none,       // special color denoting an unidentified color.
  any,        // special color denoting any color.
  any_except, // special color denoting any color with except.

  highload,
  no_highload,
  ssr,
  ssr_allow_db,
  has_db_access,
  api,
  api_callback,
  api_allow_curl,
  has_curl,
  message_internals,
  message_module,
  danger_zone,

  _count // auxiliary color denotes the number of colors.
};

// parse_color converts the string representation of a color
// to an color_t enumeration value.
color_t parse_color(const std::string &str);

// color_to_string converts color to its string representation.
std::string color_to_string(color_t color);

/**
 * ColorContainer is a class for conveniently working with a set of colors,
 * where order is **important**.
 */
class ColorContainer {
  using ColorsRaw = std::array<color_t, static_cast<size_t>(color_t::_count)>;

private:
  ColorsRaw data{};
  size_t count{0};

public:
  const ColorsRaw &colors();
  void add(const color_t &color);
  size_t size() const noexcept;
  bool empty() const noexcept;
  color_t operator[](size_t index) const;
  std::string to_string() const;
};

struct Node;

/**
 * NodeContainer is a class for conveniently working
 * with a map<color,Node*>, where order is **not important**.
 */
class NodeContainer {
public:
  static const auto Size = static_cast<size_t>(color_t::_count);

private:
  Node *data[static_cast<size_t>(color_t::_count)]{nullptr};
  size_t count{0};

public:
  void add(Node *node);
  Node *get(color_t color) const;
  bool has(color_t color) const noexcept;
  void del(color_t color);
  size_t size() const noexcept;
  bool empty() const noexcept;
  Node *operator[](size_t index) const;
};

/**
 * Node is a structure that describes a node in the RuleTree.
 */
struct Node {
  color_t color{};
  NodeContainer children;

  bool is_leaf{false};
  bool is_error{false};
  std::string message;

  // For nodes of type any_except. See RuleTree::normalize.
  std::map<Node *, NodeContainer> except_for_color;

public:
  explicit Node(color_t c)
    : color(c) {}

  Node *match(color_t clr) const {
    return this->children.get(clr);
  }

  void insert(const std::vector<color_t> &colors, size_t shift, bool error, const std::string &msg) {
    if (shift == colors.size()) {
      this->is_leaf = true;
      this->is_error = error;
      this->message = msg;
      return;
    }

    const auto cur_color = colors[colors.size() - 1 - shift];

    auto *child = this->children.get(cur_color);
    if (child == nullptr) {
      auto *child_node = new Node(cur_color);
      this->children.add(child_node);
      child_node->insert(colors, shift + 1, error, msg);
      return;
    }

    child->insert(colors, shift + 1, error, msg);
  }
};

/**
 * RuleTree is a class for storing color mixing rules.
 * The tree consists of Node nodes.
 * Leaves in the tree are indicated by nodes with
 * the is_leaf, is_error, and message values set.
 *
 * It is assumed that the tree will be built from chains of
 * colors, which either end with an error and its description,
 * or not.
 * See insert method.
 */
class RuleTree {
  Node *root_;
  std::map<std::pair<Node *, color_t>, int> need_delete;

public:
  RuleTree()
    : root_(new Node(color_t::none)) {}

  void insert(const std::vector<color_t> &colors, bool is_error = false, const std::string &message = "") {
    root_->insert(colors, 0, is_error, message);
  }

  // The normalize function converts the rule tree to a form where any (as type)
  // rules are represented as any and any_except rules.
  // any rules are rules for which there are no qualifying rules (1).
  // Example:
  //
  // (1)
  //    'green *'   => "error" // There is no clarification for the rule.
  //
  // (2)
  //    'red *'     => "error"
  //    'red green' => 1 // Rule clarifies the previous one.
  //
  // And any_except rules, which are designed to store rules for which
  // there are refinement rules (2).
  //
  // any_except rules contain an additional except field, which stores a set of colors for
  // which you do not need to call the rule, since there is a more precise rule.
  // In the previous example, this will be green.
  //
  // Thus, we get the following rules:
  //    'green *'       => "error"
  //    'red *(-green)' => "error"
  //    'red green'     => 1
  //
  // And thanks to this transformation, we can easily understand whether to check
  // the any rule (which has now become any_except) or not.
  void normalize() {
    this->normalize_tree(this->root());

    for (auto &pair : this->need_delete) {
      auto *node = pair.first.first;
      const auto color = pair.first.second;
      node->children.del(color);
    }
  }

  Node *root() const {
    return root_;
  }

private:
  void normalize_tree(Node *tree) {
    auto *any_rule = tree->match(color_t::any);
    if (any_rule == nullptr) {
      for (int i = 0; i < NodeContainer::Size; ++i) {
        auto *child = tree->children[i];
        if (child == nullptr) {
          continue;
        }

        this->normalize_tree(child);
      }
      return;
    }

    for (int i = 0; i < NodeContainer::Size; ++i) {
      auto *child = tree->children[i];
      if (child == nullptr) {
        continue;
      }

      if (child == any_rule) {
        continue;
      }

      this->normalize_subtrees(tree, any_rule, child);
      this->normalize_tree(child);
    }

    this->normalize_tree(any_rule);
  }

  void normalize_subtrees(Node *parent, Node *any_rule, Node *specific_rule) {
    for (int i = 0; i < NodeContainer::Size; ++i) {
      auto *any_rule_child = any_rule->children[i];
      if (any_rule_child == nullptr) {
        continue;
      }

      const auto *matched = specific_rule->match(any_rule_child->color);
      if (matched == nullptr) {
        continue;
      }

      const auto equal = equal_tree(any_rule_child, matched);
      if (!equal) {
        continue;
      }

      need_delete.insert(std::make_pair(std::make_pair(any_rule, matched->color), 1));

      auto *any_except_node = parent->match(color_t::any_except);
      if (any_except_node == nullptr) {
        auto *new_any_except = new Node(color_t::any_except);
        new_any_except->children.add(any_rule_child);

        auto any_except_colors = NodeContainer();
        any_except_colors.add(specific_rule);

        new_any_except->except_for_color.insert(std::make_pair(any_rule_child, any_except_colors));

        parent->children.add(new_any_except);
      } else {
        auto except = any_except_node->except_for_color.find(any_rule_child);
        if (except != any_except_node->except_for_color.end()) {
          except->second.add(specific_rule);
        } else {
          auto any_except_colors = NodeContainer();
          any_except_colors.add(specific_rule);

          any_except_node->except_for_color.insert(std::make_pair(any_rule_child, any_except_colors));
        }

        any_except_node->children.add(any_rule_child);
      }
    }
  }

  static bool equal_tree(const Node *first, const Node *second) {
    if (first->color != second->color) {
      return false;
    }

    if (first->children.size() != second->children.size()) {
      return false;
    }

    for (int i = 0; i < NodeContainer::Size; ++i) {
      const auto *first_child = first->children[i];
      const auto *second_child = second->children[i];
      if (first_child == nullptr && second_child == nullptr) {
        continue;
      }

      if (first_child != second_child) {
        return false;
      }

      const auto children_equal = equal_tree(first_child, second_child);
      if (!children_equal) {
        return false;
      }
    }

    return true;
  }
};

} // namespace function_palette
