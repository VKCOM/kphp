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
  none, // special color denoting an unsettled color.
  any,  // special color denoting any color.

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
 * Colors is a class that describes a container
 * that stores the colors of a function.
 */
class Colors {
  using ColorsRaw = std::array<Color, static_cast<size_t>(Color::_count)>;

private:
  ColorsRaw colors_{};
  int count{0};

public:
  const ColorsRaw &colors() {
    return this->colors_;
  }

  void add(const Color &color) {
    if (this->count >= static_cast<size_t>(Color::_count)) {
      return;
    }
    this->colors_[this->count] = color;
    this->count++;
  }

  int length() const noexcept {
    return this->count;
  }

  bool empty() const noexcept {
    return this->count == 0;
  }

  std::string to_string() const {
    auto *file = stdout;
    const auto with_color = stage::should_be_colored(file);

    std::string desc;

    desc += "colors: {";

    for (int i = 0; i < this->count; ++i) {
      const auto color = this->colors_[i];
      const auto color_str = Colors::color_to_string(color);

      desc += with_color ? TermStringFormat::paint_green(color_str) : color_str;

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

/**
 * PaletteNode is a structure that describes a node in the PaletteTree tree.
 */
struct PaletteNode {
  Color color{};
  std::vector<PaletteNode *> children;

  bool is_leaf{false};
  bool is_error{false};
  std::string message;

public:
  explicit PaletteNode(Color c)
    : color(c) {}

  PaletteNode *match(Color clr) {
    for (auto &child : this->children) {
      if (child->color == clr) {
        return child;
      }
    }

    return nullptr;
  }

  void insert(const std::vector<Color> &colors, size_t shift, bool error, const std::string &msg) {
    if (shift == colors.size()) {
      this->is_leaf = true;
      this->is_error = error;
      this->message = msg;
      return;
    }

    bool has_child = false;

    for (auto &child : this->children) {
      if (child->color == colors[colors.size() - 1 - shift]) {
        has_child = true;
        child->insert(colors, shift + 1, error, msg);
        break;
      }
    }

    if (!has_child) {
      auto *child_node = new PaletteNode(colors[colors.size() - 1 - shift]);
      this->children.push_back(child_node);
      child_node->insert(colors, shift + 1, error, msg);
    }
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

public:
  PaletteTree()
    : root_(new PaletteNode(Color::none)) {}

  void insert(const std::vector<Color> &colors, bool is_error = false, const std::string &message = "") const {
    root_->insert(colors, 0, is_error, message);
  }

  PaletteNode *root() const {
    return root_;
  }
};
