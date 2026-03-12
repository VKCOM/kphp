// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "common/unicode/unicode-utils.h"

#include <algorithm>
#include <assert.h>
#include <cstddef>
#include <functional>
#include <iterator>
#include <stdlib.h>
#include <string.h>

#include "auto/common/unicode-utils-auto.h"

#include "common/unicode/utf8-utils.h"

/* Search generated ranges for specified character */
static int binary_search_ranges(const int* ranges, int r, int code, void (*assertf)(bool)) {
  if ((unsigned int)code > 0x10ffff) {
    return 0;
  }

  int l = 0;
  while (l < r) {
    int m = ((l + r + 2) >> 2) << 1;
    if (ranges[m] <= code) {
      l = m;
    } else {
      r = m - 2;
    }
  }

  int t = ranges[l + 1];
  if (t < 0) {
    return code - ranges[l] + (~t);
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
    assertf(false);
  }
  return 0;
}

/* Convert character to upper case */
int unicode_toupper(int code) {
  if ((unsigned int)code < (unsigned int)TABLE_SIZE) {
    return to_upper_table[code];
  } else {
    return binary_search_ranges(to_upper_table_ranges, to_upper_table_ranges_size, code, [](bool condition) { assert(condition); });
  }
}

/* Convert character to lower case */
int unicode_tolower(int code) {
  if ((unsigned int)code < (unsigned int)TABLE_SIZE) {
    return to_lower_table[code];
  } else {
    return binary_search_ranges(to_lower_table_ranges, to_lower_table_ranges_size, code, [](bool condition) { assert(condition); });
  }
}

inline constexpr int32_t WHITESPACE_CODE_POINT{static_cast<int32_t>(' ')};
inline constexpr int32_t PLUS_CODE_POINT{static_cast<int32_t>('+')};

/* Prepares unicode 0-terminated string input for search,
   leaving only digits and letters with diacritics.
   Length of string can decrease.
   Returns length of result. */
size_t prepare_search_string(int32_t* code_points, void (*assertf)(bool)) noexcept {
  size_t output_size{};
  for (size_t i{}; code_points[i] != 0; ++i) {
    int32_t c{code_points[i]};
    int32_t new_c{};
    if (static_cast<size_t>(c) < static_cast<size_t>(TABLE_SIZE)) {
      new_c = static_cast<int32_t>(prepare_table[c]);
    } else {
      new_c = binary_search_ranges(prepare_table_ranges, prepare_table_ranges_size, c, assertf);
    }
    if (new_c != 0) {
      // we forbid 2 whitespaces after each other and starting whitespace
      if (new_c != WHITESPACE_CODE_POINT || (output_size > 0 && code_points[output_size - 1] != WHITESPACE_CODE_POINT)) {
        code_points[output_size++] = new_c;
      }
    }
  }
  if (output_size > 0 && code_points[output_size - 1] == WHITESPACE_CODE_POINT) {
    // throw out terminating whitespace
    --output_size;
  }
  code_points[output_size] = 0;
  return output_size;
}

inline size_t prepare_str_unicode(int32_t* code_points, size_t* word_start_indices, int32_t* prepared_code_points, void (*assertf)(bool)) noexcept {
  size_t code_points_length = prepare_search_string(code_points, assertf);
  code_points[code_points_length] = WHITESPACE_CODE_POINT;

  size_t words_count{};
  size_t i{};
  // looking for the beginnings of the words
  while (i < code_points_length) {
    word_start_indices[words_count++] = i;
    while (i < code_points_length && code_points[i] != WHITESPACE_CODE_POINT) {
      ++i;
    }
    ++i;
  }

  auto word_less_cmp{[&code_points](size_t x, size_t y) noexcept -> bool {
    while (code_points[x] != WHITESPACE_CODE_POINT && code_points[x] == code_points[y]) {
      ++x;
      ++y;
    }
    if (code_points[x] == WHITESPACE_CODE_POINT) {
      return code_points[y] != WHITESPACE_CODE_POINT;
    }
    if (code_points[y] == WHITESPACE_CODE_POINT) {
      return false;
    }
    return code_points[x] < code_points[y];
  }};

  std::sort(word_start_indices, std::next(word_start_indices, words_count), word_less_cmp);

  size_t uniq_words_count{};
  for (i = 0; i < words_count; ++i) {
    // drop duplicates
    if (uniq_words_count == 0 || word_less_cmp(word_start_indices[uniq_words_count - 1], word_start_indices[i])) {
      word_start_indices[uniq_words_count++] = word_start_indices[i];
    } else {
      word_start_indices[uniq_words_count - 1] = word_start_indices[i];
    }
  }

  size_t result_size{};
  // output words with '+' separator
  for (i = 0; i < uniq_words_count; ++i) {
    size_t ind{word_start_indices[i]};
    while (code_points[ind] != WHITESPACE_CODE_POINT) {
      prepared_code_points[result_size++] = code_points[ind++];
    }
    prepared_code_points[result_size++] = PLUS_CODE_POINT;
  }
  prepared_code_points[result_size++] = 0;

  assertf(result_size < MAX_NAME_SIZE);
  return result_size;
}

inline size_t clean_str_unicode(int32_t* code_points, size_t* word_start_indices, int32_t* prepared_code_points, std::byte* utf8_result,
                                void (*assertf)(bool)) noexcept {
  prepare_str_unicode(code_points, word_start_indices, prepared_code_points, assertf);

  auto length{static_cast<size_t>(put_string_utf8(prepared_code_points, reinterpret_cast<char*>(utf8_result)))};
  assertf(length < MAX_NAME_BYTES_SIZE);

  size_t i{};
  size_t result_size{};
  while (i < length) {
    char* c{reinterpret_cast<char*>(std::addressof(utf8_result[i]))};
    bool skip{!strncmp(c, "amp+", 4) || !strncmp(c, "gt+", 3) || !strncmp(c, "lt+", 3) || !strncmp(c, "quot+", 5) || !strncmp(c, "ft+", 3) ||
              !strncmp(c, "feat+", 5) ||
              (((c[0] == '1' && c[1] == '9') || (c[0] == '2' && c[1] == '0')) && ('0' <= c[2] && c[2] <= '9') && ('0' <= c[3] && c[3] <= '9') && c[4] == '+') ||
              !strncmp(c, "092+", 4) || !strncmp(c, "33+", 3) || !strncmp(c, "34+", 3) || !strncmp(c, "36+", 3) || !strncmp(c, "39+", 3) ||
              !strncmp(c, "60+", 3) || !strncmp(c, "62+", 3) || !strncmp(c, "8232+", 5) || !strncmp(c, "8233+", 5)};
    do {
      if (!skip) {
        utf8_result[result_size] = utf8_result[i];
        ++result_size;
      }
    } while (utf8_result[i++] != static_cast<std::byte>('+'));
  }
  utf8_result[result_size] = static_cast<std::byte>(0);

  return result_size;
}

size_t clean_str(const char* x, int32_t* code_points, size_t* word_start_indices, int32_t* prepared_code_points, std::byte* utf8_result,
                 void (*assertf)(bool)) {
  size_t x_len{strlen(x)};
  if (assertf == nullptr || x == NULL || x_len >= MAX_NAME_SIZE) {
    for (size_t i = 0; i < x_len; ++i) {
      utf8_result[i] = static_cast<std::byte>(x[i]);
    }
    utf8_result[x_len] = static_cast<std::byte>(0);
    return x_len;
  }

  html_string_to_utf8(x, code_points);
  return clean_str_unicode(code_points, word_start_indices, prepared_code_points, utf8_result, assertf);
}
