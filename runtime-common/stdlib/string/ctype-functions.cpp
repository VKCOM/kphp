// Compiler for PHP (aka KPHP)
// Copyright (c) 2025 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime-common/stdlib/string/ctype-functions.h"

#include <algorithm>
#include <cctype>

namespace {
bool ctype_impl(const mixed& text, int (*iswhat)(int), bool allow_digits, bool allow_minus) noexcept {
  if (text.is_string()) {
    const string& str = text.as_string();
    if (str.empty()) {
      return false;
    }
    return std::all_of(str.c_str(), str.c_str() + str.size(), [iswhat](uint8_t c) noexcept { return iswhat(c); });
  }

  if (text.is_int()) {
    const int64_t i = text.as_int();
    if (i <= 255 && i >= 0) {
      return iswhat(i);
    } else if (i >= -128 && i < 0) {
      return iswhat(i + 256);
    } else if (i >= 0) {
      return allow_digits;
    } else {
      return allow_minus;
    }
  }

  return false;
}
} // namespace

bool f$ctype_alnum(const mixed& text) noexcept {
  return ctype_impl(text, std::isalnum, true, false);
}

bool f$ctype_alpha(const mixed& text) noexcept {
  return ctype_impl(text, std::isalpha, false, false);
}

bool f$ctype_cntrl(const mixed& text) noexcept {
  return ctype_impl(text, std::iscntrl, false, false);
}

bool f$ctype_digit(const mixed& text) noexcept {
  return ctype_impl(text, std::isdigit, true, false);
}

bool f$ctype_graph(const mixed& text) noexcept {
  return ctype_impl(text, std::isgraph, true, true);
}

bool f$ctype_lower(const mixed& text) noexcept {
  return ctype_impl(text, std::islower, false, false);
}

bool f$ctype_print(const mixed& text) noexcept {
  return ctype_impl(text, std::isprint, true, true);
}

bool f$ctype_punct(const mixed& text) noexcept {
  return ctype_impl(text, std::ispunct, false, false);
}

bool f$ctype_space(const mixed& text) noexcept {
  return ctype_impl(text, std::isspace, false, false);
}

bool f$ctype_upper(const mixed& text) noexcept {
  return ctype_impl(text, std::isupper, false, false);
}

bool f$ctype_xdigit(const mixed& text) noexcept {
  return ctype_impl(text, std::isxdigit, true, false);
}
