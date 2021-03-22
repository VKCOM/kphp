// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <array>
#include <map>
#include <vector>

#include <common/termformat/termformat.h>
#include <compiler/stage.h>

enum class Color {
  none,        // special color denoting an unsettled color.
  any,         // special color denoting any color.
  any_except,  // special color denoting any color with except.

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

/**
 * FunctionColors is a class for conveniently working with a set of colors,
 * where order is **important**.
 */
class FunctionColors {
  using ColorsRaw = std::array<Color, static_cast<size_t>(Color::_count)>;

private:
  ColorsRaw data{};
  size_t count{0};

public:
  const ColorsRaw &colors() {
    return this->data;
  }

  void add(const Color &color) {
    if (this->count >= static_cast<size_t>(Color::_count)) {
      return;
    }
    this->data[this->count] = color;
    this->count++;
  }

  size_t size() const noexcept {
    return this->count;
  }

  bool empty() const noexcept {
    return this->count == 0;
  }

  Color operator[](size_t index) const {
    return this->data[index];
  }

  std::string to_string() const {
    std::string desc;

    desc += "colors: {";

    for (int i = 0; i < this->count; ++i) {
      const auto color = this->data[i];
      const auto color_str = FunctionColors::color_to_string(color);

      desc += TermStringFormat::paint_green(color_str);

      if (i != this->count - 1) {
        desc += ", ";
      }
    }

    desc += "}";

    return desc;
  }

private:
  static const std::map<std::string, Color> str2color_type;

public:
  static Color get_color_type(const std::string &str) {
    const auto it = str2color_type.find(str);
    const auto found = it != str2color_type.end();
    return found ? it->second : Color::none;
  }
  static std::string color_to_string(Color color) {
    for (const auto &pair : str2color_type) {
      if (pair.second == color) {
        return pair.first;
      }
    }
    return "";
  }
};

struct PaletteNode;

/**
 * PaletteNodeContainer is a class for conveniently working
 * with a set of colors, where order is **not important**.
 */
class PaletteNodeContainer {
public:
  static const auto Size = static_cast<size_t>(Color::_count);

private:
  PaletteNode* data[static_cast<size_t>(Color::_count)]{nullptr};
  size_t count{0};

public:
  void add(PaletteNode* node);
  PaletteNode* get(Color color) const;
  bool has(Color color) const;
  void del(Color color);
  size_t size() const;
  bool empty() const;
  PaletteNode* operator[](size_t index) const;
};

/**
 * PaletteNode is a structure that describes a node in the PaletteTree tree.
 */
struct PaletteNode {
  Color color{};
  PaletteNodeContainer children;

  bool is_leaf{false};
  bool is_error{false};
  std::string message;

  // For nodes of type any_except. See PaletteTree::normalize.
  std::map<PaletteNode*, PaletteNodeContainer> except_for_color;

public:
  explicit PaletteNode(Color c)
    : color(c) {}

  PaletteNode* match(Color clr) const {
    return this->children.get(clr);
  }

  void insert(const std::vector<Color> &colors, size_t shift, bool error, const std::string &msg) {
    if (shift == colors.size()) {
      this->is_leaf = true;
      this->is_error = error;
      this->message = msg;
      return;
    }

    const auto cur_color = colors[colors.size() - 1 - shift];

    auto* child = this->children.get(cur_color);
    if (child == nullptr) {
      auto* child_node = new PaletteNode(cur_color);
      this->children.add(child_node);
      child_node->insert(colors, shift + 1, error, msg);
      return;
    }

    child->insert(colors, shift + 1, error, msg);
  }
};

/**
 * PaletteTree is a class for storing color mixing rules.
 * The tree consists of PaletteNode nodes.
 * Leaves in the tree are indicated by nodes with
 * the is_leaf, is_error, and message values set.
 *
 * It is assumed that the tree will be built from chains of
 * colors, which either end with an error and its description,
 * or not. See insert method.
 */
class PaletteTree {
  PaletteNode *root_;
  std::map<std::pair<PaletteNode*, Color>, int> need_delete;

public:
  PaletteTree()
    : root_(new PaletteNode(Color::none)) {}

  void insert(const std::vector<Color> &colors, bool is_error = false, const std::string &message = "") {
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

    for (auto& pair : this->need_delete) {
      auto* node = pair.first.first;
      const auto color = pair.first.second;
      node->children.del(color);
    }
  }

  PaletteNode *root() const {
    return root_;
  }

private:
  void normalize_tree(PaletteNode* tree) {
    auto* any_rule = tree->match(Color::any);
    if (any_rule == nullptr) {
      for (int i = 0; i < PaletteNodeContainer::Size; ++i) {
        auto* child = tree->children[i];
        if (child == nullptr) {
          continue;
        }

        this->normalize_tree(child);
      }
      return;
    }

    for (int i = 0; i < PaletteNodeContainer::Size; ++i) {
      auto* child = tree->children[i];
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

  void normalize_subtrees(PaletteNode* parent, PaletteNode* any_rule, PaletteNode* specific_rule) {
    for (int i = 0; i < PaletteNodeContainer::Size; ++i) {
      auto* any_rule_child = any_rule->children[i];
      if (any_rule_child == nullptr) {
        continue;
      }

      const auto* matched = specific_rule->match(any_rule_child->color);
      if (matched == nullptr) {
        continue;
      }

      const auto equal = equal_tree(any_rule_child, matched);
      if (!equal) {
        continue;
      }

      need_delete.insert(std::make_pair(std::make_pair(any_rule, matched->color), 1));

      auto* any_except_node = parent->match(Color::any_except);
      if (any_except_node == nullptr) {
        auto* new_any_except = new PaletteNode(Color::any_except);
        new_any_except->children.add(any_rule_child);

        auto any_except_colors = PaletteNodeContainer();
        any_except_colors.add(specific_rule);

        new_any_except->except_for_color.insert(
          std::make_pair(any_rule_child, any_except_colors)
        );

        parent->children.add(new_any_except);
      } else {
        auto except = any_except_node->except_for_color.find(any_rule_child);
        if (except != any_except_node->except_for_color.end()) {
          except->second.add(specific_rule);
        } else {
          auto any_except_colors = PaletteNodeContainer();
          any_except_colors.add(specific_rule);

          any_except_node->except_for_color.insert(
            std::make_pair(any_rule_child, any_except_colors)
          );
        }

        any_except_node->children.add(any_rule_child);
      }
    }
  }

  static bool equal_tree(const PaletteNode* first, const PaletteNode* second) {
    if (first->color != second->color) {
      return false;
    }

    if (first->children.size() != second->children.size()) {
      return false;
    }

    for (int i = 0; i < PaletteNodeContainer::Size; ++i) {
      const auto* first_child = first->children[i];
      const auto* second_child = second->children[i];
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
