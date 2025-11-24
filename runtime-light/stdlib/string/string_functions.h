// Compiler for PHP (aka KPHP)
// Copyright (c) 2025 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <span>

#include "auto/common/unicode-utils-auto.h"
#include "common/unicode/utf8-utils.h"
#include "runtime-common/core/runtime-core.h"
#include "runtime-light/stdlib/diagnostics/logs.h"

namespace string_functions_impl_ {

// TODO May be better extract MAX_NAME_SIZE to utf8-utils.h instead of copy-pasting it ?
inline constexpr size_t MAX_NAME_SIZE = 65536;
inline constexpr size_t MAX_NAME_INDICES_SIZE = MAX_NAME_SIZE + 4;
inline constexpr size_t MAX_NAME_CODE_POINTS_SIZE = MAX_NAME_SIZE + 4;
inline constexpr size_t MAX_NAME_BYTES_SIZE = MAX_NAME_SIZE * 4 + 4;

// TODO как учитывать align ?
inline constexpr size_t SOURCE_CODE_POINTS_SPAN_BEGIN = 0;
inline constexpr size_t WORD_INDICES_SPAN_BEGIN = SOURCE_CODE_POINTS_SPAN_BEGIN + sizeof(int32_t) * MAX_NAME_CODE_POINTS_SIZE;
inline constexpr size_t RESULT_CODE_POINTS_SPAN_BEGIN = WORD_INDICES_SPAN_BEGIN + sizeof(size_t) * MAX_NAME_INDICES_SIZE;
inline constexpr size_t RESULT_BYTES_SPAN_BEGIN = RESULT_CODE_POINTS_SPAN_BEGIN + sizeof(int32_t) * MAX_NAME_CODE_POINTS_SIZE;
inline constexpr size_t RESULT_BYTES_SPAN_END = RESULT_BYTES_SPAN_BEGIN + sizeof(int32_t) * MAX_NAME_BYTES_SIZE;

inline constexpr int32_t MAX_UTF8_CODE_POINT{0x10ffff};

/* Search generated ranges for specified character */
inline int32_t binary_search_ranges(int32_t code) noexcept {
  if (code > MAX_UTF8_CODE_POINT) {
    return 0;
  }

  size_t l{0};
  size_t r{prepare_table_ranges_size};
  while (l < r) {
    // TODO verify this formula
    size_t m{((l + r + 2) >> 2) << 1};
    if (prepare_table_ranges[m] <= code) {
      l = m;
    } else {
      // TODO why `- 2` ?
      r = m - 2;
    }
  }

  // prepare_table_ranges[l]     - key
  // prepare_table_ranges[l + 1] - value
  int32_t t{prepare_table_ranges[l + 1]};
  if (t < 0) {
    // TODO блять что это ??
    return code - prepare_table_ranges[l] + (~t);
  }
  if (t <= 0x10ffff) {
    return t;
  }
  switch (t - 0x200000) {
  case 0:
    // TODO а это
    return (code & -2);
  case 1:
    // TODO и это ещё
    return (code | 1);
  case 2:
    // TODO ??
    return ((code - 1) | 1);
  default:
    k2::exit(1);
  }
}

inline constexpr int32_t WHITESPACE{static_cast<int32_t>(' ')};
inline constexpr int32_t PLUS{static_cast<int32_t>('+')};

// TODO naming
/* Prepares unicode 0-terminated string input for search,
   leaving only digits and letters with diacritics.
   Length of string can decrease.
   Returns length of result. */
inline void prepare_search_string(std::span<int32_t>& code_points) noexcept {
  size_t output_size{};
  for (size_t i{}; i < code_points.size(); ++i) {
    int32_t c{code_points[i]};
    int32_t new_c{};
    if (static_cast<size_t>(c) < static_cast<size_t>(TABLE_SIZE)) {
      // Таблица каких-то преобразований для первых 1280 символов
      new_c = static_cast<int32_t>(prepare_table[c]);
    } else {
      // Бинпоиск по мапе преобразований сразу целых range'ей
      // prepare_table_ranges - мапа, закодированная в массиве, ага
      new_c = binary_search_ranges(c);
    }
    // TODO replace with `new_c != 0` ?
    if (new_c) {
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
  code_points = code_points.subspan(output_size);
}

// TODO naming
inline std::span<int32_t> prepare_str_unicode(std::span<int32_t> code_points) noexcept {
  prepare_search_string(code_points);
  code_points[code_points.size()] = WHITESPACE;

  auto* word_indices_begin{reinterpret_cast<size_t*>(RuntimeContext::get().static_SB.buffer()[WORD_INDICES_SPAN_BEGIN])};
  std::span<size_t> word_start_indices{word_indices_begin, MAX_NAME_INDICES_SIZE}; // indices of first char of every word in `code_points`.
  size_t words_count{};
  size_t i{};
  // looking for the beginnings of the words
  while (i < code_points.size()) {
    word_start_indices[words_count++] = i;
    while (i < code_points.size() && code_points[i] != WHITESPACE) {
      i++;
    }
    i++;
  }
  word_start_indices = word_start_indices.subspan(words_count);

  auto word_less_cmp{[&code_points](size_t x, size_t y) noexcept -> bool {
    while (code_points[x] != WHITESPACE && code_points[x] == code_points[y]) {
      ++x;
      ++y;
    }
    if (code_points[x] == WHITESPACE) {
      return code_points[y] != WHITESPACE;
    }
    if (code_points[y] == WHITESPACE) {
      return false;
    }
    return code_points[x] < code_points[y];
  }};

  std::sort(word_start_indices.begin(), word_start_indices.end(), word_less_cmp);

  size_t uniq_words_count{};
  for (i = 0; i < words_count; i++) {
    // drop duplicates
    if (uniq_words_count == 0 || word_less_cmp(word_start_indices[uniq_words_count - 1], word_start_indices[i])) {
      word_start_indices[uniq_words_count++] = word_start_indices[i];
    } else {
      word_start_indices[uniq_words_count - 1] = word_start_indices[i];
    }
  }

  auto* result_begin{reinterpret_cast<int32_t*>(RuntimeContext::get().static_SB.buffer()[RESULT_CODE_POINTS_SPAN_BEGIN])};
  std::span<int32_t> result{result_begin, MAX_NAME_CODE_POINTS_SIZE};
  size_t result_size{};
  // output words with '+' separator
  for (i = 0; i < uniq_words_count; i++) {
    size_t ind{word_start_indices[i]};
    while (code_points[ind] != WHITESPACE) {
      result[result_size++] = code_points[ind++];
    }
    result[result_size++] = PLUS;
  }
  result[result_size++] = 0;

  kphp::log::assertion(result_size < MAX_NAME_SIZE);
  result = result.subspan(result_size);
  return result;
}

// TODO naming
inline std::span<const std::byte> clean_str_unicode(std::span<int32_t> code_points) noexcept {
  std::span<int32_t> prepared_code_points{prepare_str_unicode(code_points)};

  auto* utf8_result_begin{reinterpret_cast<std::byte*>(prepared_code_points.begin()[RESULT_CODE_POINTS_SPAN_BEGIN])};
  std::span<std::byte> utf8_result{utf8_result_begin, MAX_NAME_BYTES_SIZE};
  // put_string_utf8 можно использовать в runtime-light
  auto length{static_cast<size_t>(put_string_utf8(prepared_code_points.data(), reinterpret_cast<char*>(utf8_result.data())))};
  kphp::log::assertion(length < utf8_result.size());
  utf8_result = utf8_result.subspan(length);

  size_t i{};
  size_t result_size{};
  while (i < utf8_result.size()) {
    char* c{reinterpret_cast<char*>(&utf8_result[i])};
    bool skip{!std::strncmp(c, "amp+", 4) || !std::strncmp(c, "gt+", 3) || !std::strncmp(c, "lt+", 3) || !std::strncmp(c, "quot+", 5) ||
              !std::strncmp(c, "ft+", 3) || !std::strncmp(c, "feat+", 5) ||
              // скипаем год ?
              (((c[0] == '1' && c[1] == '9') || (c[0] == '2' && c[1] == '0')) && ('0' <= c[2] && c[2] <= '9') && ('0' <= c[3] && c[3] <= '9') && c[4] == '+') ||
              !std::strncmp(c, "092+", 4) || !std::strncmp(c, "33+", 3) || !std::strncmp(c, "34+", 3) || !std::strncmp(c, "36+", 3) ||
              !std::strncmp(c, "39+", 3) || !std::strncmp(c, "60+", 3) || !std::strncmp(c, "62+", 3) || !std::strncmp(c, "8232+", 5) ||
              !std::strncmp(c, "8233+", 5)};
    do {
      if (!skip) {
        utf8_result[result_size] = utf8_result[i];
        ++result_size;
      }
    } while (utf8_result[i++] != static_cast<std::byte>('+'));
  }
  utf8_result[result_size] = static_cast<std::byte>(0);

  return utf8_result;
}

inline std::span<const std::byte> prepare_search_query_impl(std::span<const std::byte> x) noexcept {
  if (x.empty() || x.size() >= MAX_NAME_SIZE) {
    return x;
  }

  RuntimeContext::get().static_SB.clean().reserve(RESULT_BYTES_SPAN_END);

  // TODO провалидировать с ребятами ебучую разметку статик буфера
  auto* utf8_code_points_begin{reinterpret_cast<int32_t*>(RuntimeContext::get().static_SB.buffer())};
  std::span<int32_t> utf8_code_points{
      utf8_code_points_begin,
      MAX_NAME_CODE_POINTS_SIZE,
  };

  // html_string_to_utf8 можно полностью использовать в runtime-light
  html_string_to_utf8(reinterpret_cast<const char*>(x.data()), utf8_code_points.data());
  return clean_str_unicode(utf8_code_points);
}

} // namespace string_functions_impl_

inline string f$prepare_search_query(const string& query) noexcept {
  std::span<const std::byte> s{
      string_functions_impl_::prepare_search_query_impl({reinterpret_cast<const std::byte*>(query.c_str()), static_cast<size_t>(query.size())})};
  return {reinterpret_cast<const char*>(s.data()), static_cast<string::size_type>(s.size())};
}
