// Compiler for PHP (aka KPHP)
// Copyright (c) 2025 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime-common/stdlib/system/system-functions.h"

#include <cstring>

#include "runtime-common/core/runtime-core.h"
#include "runtime-common/core/utils/kphp-assert-core.h"


string f$escapeshellarg(const string& arg) noexcept {
  php_assert(std::strlen(arg.c_str()) == arg.size() && "Input string contains NULL bytes");

  string result;
  result.reserve_at_least(arg.size() + 2);
  result.push_back('\'');

  for (std::size_t i = 0; i < arg.size(); ++i) {
    if (arg[i] == '\'') {
      result.push_back('\'');
      result.push_back('\\');
      result.push_back('\'');
    }
    result.push_back(arg[i]);
  }

  result.push_back('\'');
  return result;
}

string f$escapeshellcmd(const string& cmd) noexcept {
  php_assert(std::strlen(cmd.c_str()) == cmd.size() && "Input string contains NULL bytes");

  string result;
  result.reserve_at_least(cmd.size());

  const char* p = nullptr;

  for (std::size_t i = 0; i < cmd.size(); ++i) {
    switch (cmd[i]) {
    case '"':
    case '\'':
      if (!p && (p = static_cast<const char*>(std::memchr(cmd.c_str() + i + 1, cmd[i], cmd.size() - i - 1)))) {
        // noop
      } else if (p && *p == cmd[i]) {
        p = nullptr;
      } else {
        result.push_back('\\');
      }
      result.push_back(cmd[i]);
      break;
    case '#': // this is character-set independent
    case '&':
    case ';':
    case '`':
    case '|':
    case '*':
    case '?':
    case '~':
    case '<':
    case '>':
    case '^':
    case '(':
    case ')':
    case '[':
    case ']':
    case '{':
    case '}':
    case '$':
    case '\\':
    case '\x0A':
    case '\xFF':
      result.push_back('\\');
      [[fallthrough]];
    default:
      result.push_back(cmd[i]);
    }
  }

  return result;
}
