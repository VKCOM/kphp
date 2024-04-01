// Compiler for PHP (aka KPHP)
// Copyright (c) 2021 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "compiler/function-colors.h"

#include "common/algorithms/string-algorithms.h"

using namespace function_palette;

PaletteRule::PaletteRule(std::vector<color_t> &&colors, std::string error)
  : colors(std::move(colors))
  , error(std::move(error)) {

  for (color_t color : this->colors) {
    mask |= color;
  }
}

std::string PaletteRule::as_human_readable(const Palette &palette) const {
  return vk::join(colors, " ", [palette](auto color) { return palette.get_name_by_color(color); });
}

// --------------------------------------------

Palette::Palette() {
  color_names_mapping["transparent"] = special_color_transparent;
  color_names_mapping["remover"] = special_color_remover;
}

color_t Palette::register_color_name(const std::string &color_name) {
  auto found = color_names_mapping.find(color_name);
  if (found != color_names_mapping.end()) {
    return found->second;
  }

  int bit_shift = color_names_mapping.size() - 1; // user-defined colors are 1<<1, 1<<2, and so on
  color_t color = 1ULL << bit_shift;

  color_names_mapping[color_name] = color;
  kphp_error(color_names_mapping.size() != 64, "Reached limit of colors in palette — more than 64 colors");
  return color;
}

color_t Palette::get_color_by_name(const std::string &color_name) const {
  const auto found = color_names_mapping.find(color_name);
  kphp_assert(found != color_names_mapping.end());
  return found->second;
}

std::string Palette::get_name_by_color(color_t color) const {
  for (const auto &pair : color_names_mapping) {
    if (pair.second == color) {
      return pair.first;
    }
  }
  return std::to_string(color);
}

// --------------------------------------------

void ColorContainer::add(color_t color) {
  if (color == special_color_transparent) {
    return;
  }
  // append color to the end of forward list
  // a bit inconvenient, but we use forward_list intentionally to consume only 8 bytes on emptiness (the most common case)
  auto before_end = std::next(sep_colors.before_begin(), std::distance(sep_colors.begin(), sep_colors.end()));
  sep_colors.emplace_after(before_end, color);
}

bool ColorContainer::contains(color_t color) const {
  return std::any_of(sep_colors.begin(), sep_colors.end(), [=](color_t c) { return c == color; });
}

std::string ColorContainer::to_string(const Palette &palette, colors_mask_t with_highlights) const {
  std::string desc;

  for (color_t color : sep_colors) {
    if (with_highlights == 0 || with_highlights & color) {
      desc += "@";
      desc += palette.get_name_by_color(color);
    }
  }

  return desc;
}
