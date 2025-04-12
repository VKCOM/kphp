// Compiler for PHP (aka KPHP)
// Copyright (c) 2021 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <forward_list>
#include <map>

#include "common/termformat/termformat.h"
#include "compiler/stage.h"

namespace function_palette {

using color_t = uint64_t;
using colors_mask_t = uint64_t;

constexpr color_t special_color_transparent = 0;        // all functions without any colors are transparent: C + transparent = C
constexpr color_t special_color_remover = 1ULL << 63;   // @kphp-color remover works so: C + remover = transparent

class Palette;

// rules are representation of human-written "api has-curl" => "error text" or "api allow-curl has-curl" => 1
// all colors are pre-converted to numeric while reading strings
struct PaletteRule {
  std::vector<color_t> colors;
  colors_mask_t mask{0};
  std::string error;

  PaletteRule(std::vector<color_t> &&colors, std::string error);

  std::string as_human_readable(const Palette &palette) const;

  bool is_error() const {
    return !error.empty();
  }

  bool contains_in(colors_mask_t colors_mask) const {
    return (mask & colors_mask) == mask;
  }
};

// a ruleset is a group of rules, order is important
// typically, it looks like one error rule and some "exceptions" — more specific color chains with no error
using PaletteRuleset = std::vector<PaletteRule>;

// a palette is a group of rulesets
// all colors are stored as numbers, not as strings:
// they are numbers 1<<n, as we want to use bitmasks to quickly test whether to check a rule for callstack
class Palette {
  std::vector<PaletteRuleset> rulesets;
  std::map<std::string, color_t> color_names_mapping;

public:
  Palette();

  color_t register_color_name(const std::string &color_name);

  void add_ruleset(PaletteRuleset &&ruleset) {
    rulesets.emplace_back(std::move(ruleset));
  }

  const std::vector<PaletteRuleset> &get_rulesets() const {
    return rulesets;
  }

  bool color_exists(const std::string &color_name) const {
    return color_names_mapping.find(color_name) != color_names_mapping.end();
  }

  color_t get_color_by_name(const std::string &color_name) const;
  std::string get_name_by_color(color_t color) const;
};

// a class containing colors after @kphp-color parsing above each function (order is important)
class ColorContainer {
  std::forward_list<color_t> sep_colors; // empty almost everywhere

public:
  ColorContainer() = default;

  bool empty() const { return sep_colors.empty(); }

  auto begin() const { return sep_colors.begin(); }
  auto end() const { return sep_colors.end(); }

  void add(color_t color);
  bool contains(color_t color) const;

  std::string to_string(const Palette &palette, colors_mask_t with_highlights) const;
};

} // namespace function_palette
