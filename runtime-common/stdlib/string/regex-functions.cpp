// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime-common/stdlib/string/regex-functions.h"

#include "runtime-common/core/runtime-core.h"

string f$preg_quote(const string &str, const string &delimiter) noexcept {
  auto &static_SB{RuntimeContext::get().static_SB};

  const string::size_type len{str.size()};
  static_SB.clean().reserve(4 * len);
  for (string::size_type i = 0; i < len; i++) {
    switch (str[i]) {
      case '.':
      case '\\':
      case '+':
      case '*':
      case '?':
      case '[':
      case '^':
      case ']':
      case '$':
      case '(':
      case ')':
      case '{':
      case '}':
      case '=':
      case '!':
      case '>':
      case '<':
      case '|':
      case ':':
      case '-':
      case '#':
        static_SB.append_char('\\');
        static_SB.append_char(str[i]);
        break;
      case '\0':
        static_SB.append_char('\\');
        static_SB.append_char('0');
        static_SB.append_char('0');
        static_SB.append_char('0');
        break;
      default:
        if (!delimiter.empty() && str[i] == delimiter[0]) {
          static_SB.append_char('\\');
        }
        static_SB.append_char(str[i]);
        break;
    }
  }

  return static_SB.str();
}
