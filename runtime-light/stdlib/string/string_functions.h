// Compiler for PHP (aka KPHP)
// Copyright (c) 2025 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <memory>
#include <span>

#include "common/unicode/unicode-utils.h"
#include "common/unicode/utf8-utils.h"
#include "runtime-common/core/runtime-core.h"
#include "runtime-common/stdlib/string/string-context.h"
#include "runtime-light/k2-platform/k2-api.h"
#include "runtime-light/stdlib/diagnostics/logs.h"

namespace string_functions_impl_ {

inline constexpr size_t __SOURCE_CODE_POINTS_SPAN_SIZE_IN_BYTES = sizeof(int32_t) * MAX_NAME_CODE_POINTS_SIZE;
inline constexpr size_t __WORD_INDICES_SPAN_SIZE_IN_BYTES = sizeof(size_t) * MAX_NAME_CODE_POINTS_SIZE;
inline constexpr size_t __RESULT_CODE_POINTS_SPAN_SIZE_IN_BYTES = sizeof(int32_t) * MAX_NAME_CODE_POINTS_SIZE;
inline constexpr size_t __RESULT_BYTES_SPAN_SIZE_IN_BYTES = sizeof(std::byte) * MAX_NAME_BYTES_SIZE;

static_assert(__SOURCE_CODE_POINTS_SPAN_SIZE_IN_BYTES + __WORD_INDICES_SPAN_SIZE_IN_BYTES + __RESULT_CODE_POINTS_SPAN_SIZE_IN_BYTES +
                  __RESULT_BYTES_SPAN_SIZE_IN_BYTES <
              StringLibContext::STATIC_BUFFER_LENGTH);

inline constexpr size_t SOURCE_CODE_POINTS_SPAN_BEGIN = 0;
inline constexpr size_t WORD_INDICES_SPAN_BEGIN = SOURCE_CODE_POINTS_SPAN_BEGIN + __SOURCE_CODE_POINTS_SPAN_SIZE_IN_BYTES;
inline constexpr size_t RESULT_CODE_POINTS_SPAN_BEGIN = WORD_INDICES_SPAN_BEGIN + __WORD_INDICES_SPAN_SIZE_IN_BYTES;
inline constexpr size_t RESULT_BYTES_SPAN_BEGIN = RESULT_CODE_POINTS_SPAN_BEGIN + __RESULT_CODE_POINTS_SPAN_SIZE_IN_BYTES;

inline constexpr int32_t MAX_UTF8_CODE_POINT{0x10ffff};

inline constexpr int32_t WHITESPACE{static_cast<int32_t>(' ')};
inline constexpr int32_t PLUS{static_cast<int32_t>('+')};

/* Search generated ranges for specified character */
int32_t binary_search_ranges(int32_t code) noexcept;

/* Prepares unicode 0-terminated string input for search,
   leaving only digits and letters with diacritics.
   Length of string can decrease.
   Returns length of result. */
void prepare_search_string(std::span<int32_t>& code_points) noexcept;

inline std::span<int32_t> prepare_str_unicode(std::span<int32_t> code_points) noexcept {
  prepare_search_string(code_points);
  code_points[code_points.size()] = WHITESPACE;

  auto& string_lib_ctx{StringLibContext::get()};
  auto* word_indices_begin{reinterpret_cast<size_t*>(std::next(string_lib_ctx.static_buf.get(), WORD_INDICES_SPAN_BEGIN))};
  // indices of first char of every word in `code_points`.
  std::span<size_t> word_start_indices{word_indices_begin, MAX_NAME_CODE_POINTS_SIZE};
  size_t words_count{};
  size_t i{};
  // looking for the beginnings of the words
  while (i < code_points.size()) {
    word_start_indices[words_count++] = i;
    while (i < code_points.size() && code_points[i] != WHITESPACE) {
      ++i;
    }
    ++i;
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
  for (i = 0; i < words_count; ++i) {
    // drop duplicates
    if (uniq_words_count == 0 || word_less_cmp(word_start_indices[uniq_words_count - 1], word_start_indices[i])) {
      word_start_indices[uniq_words_count++] = word_start_indices[i];
    } else {
      word_start_indices[uniq_words_count - 1] = word_start_indices[i];
    }
  }

  auto* result_begin{reinterpret_cast<int32_t*>(std::next(string_lib_ctx.static_buf.get(), RESULT_CODE_POINTS_SPAN_BEGIN))};
  std::span<int32_t> result{result_begin, MAX_NAME_CODE_POINTS_SIZE};
  size_t result_size{};
  // output words with '+' separator
  for (i = 0; i < uniq_words_count; ++i) {
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

inline std::span<const std::byte> clean_str_unicode(std::span<int32_t> source_code_points) noexcept {
  std::span<int32_t> prepared_code_points{prepare_str_unicode(source_code_points)};

  auto& string_lib_ctx{StringLibContext::get()};
  auto* utf8_result_begin{reinterpret_cast<std::byte*>(std::next(string_lib_ctx.static_buf.get(), RESULT_BYTES_SPAN_BEGIN))};
  std::span<std::byte> utf8_result{utf8_result_begin, MAX_NAME_BYTES_SIZE};
  auto length{static_cast<size_t>(put_string_utf8(prepared_code_points.data(), reinterpret_cast<char*>(utf8_result.data())))};
  kphp::log::assertion(length < utf8_result.size());
  utf8_result = utf8_result.subspan(length);

  size_t i{};
  size_t result_size{};
  while (i < utf8_result.size()) {
    char* c{reinterpret_cast<char*>(std::addressof(utf8_result[i]))};
    bool skip{!std::strncmp(c, "amp+", 4) || !std::strncmp(c, "gt+", 3) || !std::strncmp(c, "lt+", 3) || !std::strncmp(c, "quot+", 5) ||
              !std::strncmp(c, "ft+", 3) || !std::strncmp(c, "feat+", 5) ||
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

  auto& string_lib_ctx{StringLibContext::get()};
  auto* source_code_points_begin{reinterpret_cast<int32_t*>(std::next(string_lib_ctx.static_buf.get(), SOURCE_CODE_POINTS_SPAN_BEGIN))};
  std::span<int32_t> source_code_points{
      source_code_points_begin,
      MAX_NAME_CODE_POINTS_SIZE,
  };

  html_string_to_utf8(reinterpret_cast<const char*>(x.data()), source_code_points.data());
  return clean_str_unicode(source_code_points);
}

} // namespace string_functions_impl_

inline string f$prepare_search_query(const string& query) noexcept {
  std::span<const std::byte> s{
      string_functions_impl_::prepare_search_query_impl({reinterpret_cast<const std::byte*>(query.c_str()), static_cast<size_t>(query.size())})};
  return {reinterpret_cast<const char*>(s.data()), static_cast<string::size_type>(s.size())};
}
