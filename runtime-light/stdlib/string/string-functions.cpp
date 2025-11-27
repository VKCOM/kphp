// Compiler for PHP (aka KPHP)
// Copyright (c) 2025 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime-light/stdlib/string/string-functions.h"

#include <cstddef>
#include <cstdint>
#include <span>

#include "auto/common/unicode-utils-auto.h"
#include "runtime-light/k2-platform/k2-api.h"

namespace string_functions_impl_ {

/* Search generated ranges for specified character */
int32_t binary_search_ranges(int32_t code) noexcept {
  if (code > MAX_UTF8_CODE_POINT) {
    return 0;
  }

  size_t l{0};
  size_t r{prepare_table_ranges_size};
  while (l < r) {
    size_t m{((l + r + 2) >> 2) << 1};
    if (prepare_table_ranges[m] <= code) {
      l = m;
    } else {
      r = m - 2;
    }
  }

  // prepare_table_ranges[l]     - key
  // prepare_table_ranges[l + 1] - value
  int32_t t{prepare_table_ranges[l + 1]};
  if (t < 0) {
    return code - prepare_table_ranges[l] + (~t);
  }
  if (t <= 0x10ffff) {
    return t;
  }
  switch (t - 0x200000) {
  case 0:
    return (code & -2);
  case 1:
    return (code | 1);
  case 2:
    return ((code - 1) | 1);
  default:
    k2::exit(1);
  }
}

/* Prepares unicode 0-terminated string input for search,
   leaving only digits and letters with diacritics.
   Length of string can decrease.
   Returns length of result. */
void prepare_search_string(std::span<int32_t>& code_points) noexcept {
  size_t output_size{};
  for (size_t i{}; code_points[i] != 0; ++i) {
    int32_t c{code_points[i]};
    int32_t new_c{};
    if (static_cast<size_t>(c) < static_cast<size_t>(TABLE_SIZE)) {
      new_c = static_cast<int32_t>(prepare_table[c]);
    } else {
      new_c = binary_search_ranges(c);
    }
    if (new_c != 0) {
      // we forbid 2 whitespaces after each other and starting whitespace
      if (new_c != WHITESPACE || (output_size > 0 && code_points[output_size - 1] != WHITESPACE)) {
        code_points[output_size++] = new_c;
      }
    }
  }
  if (output_size > 0 && code_points[output_size - 1] == WHITESPACE) {
    // throw out terminating whitespace
    --output_size;
  }
  code_points[output_size] = 0;
  code_points = code_points.first(output_size);
}

} // namespace string_functions_impl_
