// Compiler for PHP (aka KPHP)
// Copyright (c) 2025 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime-common/stdlib/hash/hash-functions.h"

bool f$hash_equals(const string &known_string, const string &user_string) noexcept {
  if (known_string.size() != user_string.size()) {
    return false;
  }
  int result = 0;
  // This is security sensitive code. Do not optimize this for speed
  // Compares two strings using the same time whether they're equal or not
  // A difference in length will leak
  for (int i = 0; i != known_string.size(); ++i) {
    result |= known_string[i] ^ user_string[i];
  }
  return !result;
}
