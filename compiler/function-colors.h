// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <array>
#include <map>
#include <unordered_map>
#include <vector>

#include <common/termformat/termformat.h>
#include <compiler/stage.h>

namespace function_palette {

using color_t = uint64_t;
using colors_t = color_t;
const auto colors_size = sizeof(color_t) * 8 - 1;

enum special_colors {
  any = color_t(1) << 63,
  none = 0,
};

using colors_num = std::map<std::string, function_palette::color_t>;

size_t count_significant_bits(uint64_t n);

class Palette;

struct Rule {
  size_t id;
  std::vector<color_t> colors;
  colors_t mask{0};

  std::string error;
  size_t count_any{0};

private:
  static size_t counter;

public:
  Rule(std::vector<color_t> &&colors, const std::string &error)
    : colors(colors), error(error) {
    for (const auto& color : colors) {
      mask |= color;
      if (color == special_colors::any) {
        count_any++;
      }
    }

    id = counter;
    counter++;
  }

  std::string to_string(const Palette &palette) const;

  bool match(const std::vector<function_palette::colors_t> &match_colors) const {
    const auto match_mask = calc_mask(match_colors);
    if (!contains_in(match_mask)) {
      return false;
    }

    return match_two_vectors(colors, match_colors);
  }

  static bool match_two_vectors(const std::vector<function_palette::colors_t> &first, const std::vector<function_palette::colors_t> &second);

  bool is_error() const {
    return !error.empty();
  }

  bool contains_in(colors_t colors_mask) const {
    const auto res_mask = mask & colors_mask;

    if (count_any > 0) {
      if (res_mask == colors_mask) {
        return true;
      }

      const auto count_bits = count_significant_bits(res_mask);
      if (count_bits == count_any) {
        return true;
      }
    }

    return res_mask == mask;
  }

private:
  static colors_t calc_mask(const std::vector<function_palette::colors_t> &colors) {
    colors_t mask = 0;
    for (const auto &color : colors) {
      mask |= color;
    }
    return mask;
  }
};

class PaletteGroup {
  std::vector<Rule> rules_;

public:
  void add_rule(Rule &&rule) {
    rules_.push_back(rule);
  }

  void add_rule(const Rule &rule) {
    rules_.push_back(rule);
  }

  size_t count() const {
    return rules_.size();
  }

  const std::vector<Rule> &rules() const {
    return rules_;
  }
};

class Palette {
  std::vector<PaletteGroup> groups_;
  function_palette::colors_num color_nums;

public:
  void set_color_nums(colors_num &&colorNums) {
    color_nums = colorNums;
  }

  void add_group(PaletteGroup &&group) {
    groups_.push_back(group);
  }

  void add_group(const PaletteGroup &group) {
    groups_.push_back(group);
  }

  const std::vector<PaletteGroup> &groups() const {
    return groups_;
  }

  // parse_color converts the string representation of a color
  // to an color_t enumeration value
  color_t parse_color(const std::string &str) const {
    const auto it = color_nums.find(str);
    const auto found = it != color_nums.end();
    return found ? it->second : static_cast<size_t>(special_colors::none);
  }

  // color_to_string converts color to its string representation
  std::string color_to_string(color_t color) const {
    for (const auto &pair : color_nums) {
      if (pair.second == color) {
        return pair.first;
      }
    }
    return "";
  }
};

/**
 * ColorContainer is a class for conveniently working with a set of colors,
 * where order is **important**.
 */
class ColorContainer {
private:
  colors_t data{};
  size_t count{0};

public:
  ColorContainer() = default;

public:
  colors_t colors();
  std::vector<color_t> sep_colors();
  void add(color_t color);
  size_t size() const noexcept;
  bool empty() const noexcept;
  std::string to_string(const Palette &palette) const;
};

} // namespace function_palette
