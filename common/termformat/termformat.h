// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <string>

class TermStringFormat {
public:
  enum color { grey, red, green, yellow, blue, magenta, cyan, white, COLORS_CNT };

  enum text_attribute { bold, dark, italic, underline, blink, inverse, strikethrough, TEXT_ATTRIBUTES_CNT };

  static std::string paint(const std::string &s, color c, bool reset_at_the_end = true);
  static std::string paint_background(const std::string &s, color c, bool reset_at_the_end = true);
  static std::string add_text_attribute(const std::string &s, text_attribute atr, bool reset_at_the_end = true);
  static std::string reset_settings(const std::string &s);
  static std::string remove_special_symbols(std::string s);
  static int get_length_without_symbols(const std::string &s);
  static bool is_terminal(FILE *file);

  static std::string paint_green(const std::string &s, bool reset_at_the_end = true);
  static std::string paint_red(const std::string &s, bool reset_at_the_end = true);
  static std::string paint_bold(const std::string &s);

private:
  static const std::string COLORS[COLORS_CNT];
  static const std::string BACKGROUND_COLORS[COLORS_CNT];
  static const std::string TEXT_ATTRIBUTES[TEXT_ATTRIBUTES_CNT];
  static const std::string RESET_SETTINGS;
  static void remove_all_occurrences(std::string &s, std::string const &t);
};
