// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "common/termformat/termformat.h"

#include <unistd.h>

using std::string;

const string TermStringFormat::COLORS[] = {
    "\033[30m", // [grey]
    "\033[31m", // [red]
    "\033[32m", // [green]
    "\033[33m", // [yellow]
    "\033[34m", // [blue]
    "\033[35m", // [magenta]
    "\033[36m", // [cyan]
    "\033[37m"  // [white]
};

const string TermStringFormat::BACKGROUND_COLORS[] = {
    "\033[40m", // [grey]
    "\033[41m", // [red]
    "\033[42m", // [green]
    "\033[43m", // [yellow]
    "\033[44m", // [blue]
    "\033[45m", // [magenta]
    "\033[46m", // [cyan]
    "\033[47m"  // [white]
};

const string TermStringFormat::TEXT_ATTRIBUTES[] = {
    "\033[1m", // [bold]
    "\033[2m", // [dark]
    "\033[3m", // [italic]
    "\033[4m", // [underline]
    "\033[5m", // [blink]
    "\033[7m", // [inverse]
    "\033[9m"  // [strikethrough]
};

const string TermStringFormat::RESET_SETTINGS = "\033[00m";

string TermStringFormat::paint(const string& s, color c, bool reset_at_the_end) {
  string res = COLORS[c] + s;
  if (reset_at_the_end) {
    res += RESET_SETTINGS;
  }
  return res;
}

string TermStringFormat::paint_background(const string& s, color c, bool reset_at_the_end) {
  string res = BACKGROUND_COLORS[c] + s;
  if (reset_at_the_end) {
    res += RESET_SETTINGS;
  }
  return res;
}

string TermStringFormat::add_text_attribute(const string& s, text_attribute atr, bool reset_at_the_end) {
  string res = TEXT_ATTRIBUTES[atr] + s;
  if (reset_at_the_end) {
    res += RESET_SETTINGS;
  }
  return res;
}

string TermStringFormat::reset_settings(const string& s) {
  return s + RESET_SETTINGS;
}

string TermStringFormat::remove_special_symbols(string s) {
  for (const auto& code : COLORS) {
    remove_all_occurrences(s, code);
  }
  for (const auto& code : TEXT_ATTRIBUTES) {
    remove_all_occurrences(s, code);
  }
  for (const auto& code : BACKGROUND_COLORS) {
    remove_all_occurrences(s, code);
  }
  remove_all_occurrences(s, RESET_SETTINGS);
  return s;
}

int TermStringFormat::get_length_without_symbols(const string& s) {
  return static_cast<int>(s.length() - remove_special_symbols(s).length());
}

bool TermStringFormat::is_terminal(FILE* file) {
  if (!file) {
    return false;
  }
  return static_cast<bool>(isatty(fileno(file)));
}

void TermStringFormat::remove_all_occurrences(string& s, const string& t) {
  size_t pos = 0;
  while ((pos = s.find(t, pos)) != string::npos) {
    s.erase(pos, t.size());
  }
}

string TermStringFormat::paint_green(const string& s, bool reset_at_the_end /* = true*/) {
  return paint(s, green, reset_at_the_end);
}

string TermStringFormat::paint_red(const string& s, bool reset_at_the_end /* = true*/) {
  return paint(s, red, reset_at_the_end);
}

std::string TermStringFormat::paint_bold(const std::string& s) {
  return add_text_attribute(s, bold);
}
