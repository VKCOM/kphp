// Compiler for PHP (aka KPHP)
// Copyright (c) 2026 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cstdint>
#include <cwchar>

#include "runtime-common/core/utils/kphp-assert-core.h"

namespace kphp::fs::details {
// this function is imported from https://github.com/php/php-src/blob/master/ext/standard/file.c,
// function php_fgetcsv_lookup_trailing_spaces
inline const char* fgetcsv_lookup_trailing_spaces(const char* ptr, size_t len, mbstate_t* ps) noexcept {
  php_assert(ps != nullptr);

  int32_t inc_len{};
  unsigned char last_chars[2]{0, 0};

  while (len > 0) {
    // SAFETY: mbrlen is thread-safe if ps != nullptr, and ps != nullptr because there is assertion at the beginning of function
    inc_len = (*ptr == '\0' ? 1 : mbrlen(ptr, len, ps)); // NOLINT
    switch (inc_len) {
    case -2:
    case -1:
      inc_len = 1;
      break;
    case 0:
      goto quit_loop;
    case 1:
    default:
      last_chars[0] = last_chars[1];
      last_chars[1] = *ptr;
      break;
    }
    ptr += inc_len;
    len -= inc_len;
  }
quit_loop:
  switch (last_chars[1]) {
  case '\n':
    if (last_chars[0] == '\r') {
      return ptr - 2;
    }
    /* fallthrough */
  case '\r':
    return ptr - 1;
  }
  return ptr;
}
} // namespace kphp::fs::details
