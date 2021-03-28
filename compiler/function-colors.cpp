// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "function-colors.h"

using namespace function_palette;

size_t function_palette::count_significant_bits(uint64_t n) {
  n -= (n >> 1) & 0x5555555555555555llu;
  n = ((n >> 2) & 0x3333333333333333llu) + (n & 0x3333333333333333llu);
  n = ((((n >> 4) + n) & 0x0F0F0F0F0F0F0F0Fllu) * 0x0101010101010101) >> 56;
  return n;
}

colors_t ColorContainer::colors() {
  return data;
}

const std::vector<color_t> &ColorContainer::sep_colors() {
  return sep_colors_;
}

void ColorContainer::add(color_t color) {
  data |= color;
  count++;
  sep_colors_.push_back(color);
}

size_t ColorContainer::size() const noexcept {
  return count;
}

bool ColorContainer::empty() const noexcept {
  return count == 0;
}

bool ColorContainer::contains(color_t color) const noexcept {
  return (data & color) == color;
}

std::string ColorContainer::to_string(const Palette &palette) const {
  std::string desc;

  desc += "colors: {";

  auto colors_output = 0;

  for (int i = 0; i < colors_size; ++i) {
    const color_t color = color_t(1) << i;
    const auto has_color = data & color;

    if (has_color) {
      const auto color_str = palette.color_to_string(color);
      desc += TermStringFormat::paint_green(color_str);
      colors_output++;

      if (colors_output != count) {
        desc += ", ";
      }
    }
  }

  desc += "}";

  return desc;
}

size_t Rule::counter = 0;

std::string Rule::to_string(const Palette &palette) const {
  std::string res = "\"";

  for (int i = 0; i < colors.size(); ++i) {
    res += palette.color_to_string(colors[i]);

    if (i != colors.size() - 1) {
      res += " ";
    }
  }

  res += "\"";

  res = TermStringFormat::paint(res, TermStringFormat::blue);

  res += " => ";

  res += error.size() == 0 ? "1" : TermStringFormat::paint(error, TermStringFormat::yellow);

  return res;
}
