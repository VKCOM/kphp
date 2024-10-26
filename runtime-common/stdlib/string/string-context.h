// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>

#include "common/mixin/not_copyable.h"
#include "runtime-common/core/runtime-core.h"

namespace string_context_impl_ {

inline constexpr auto *COLON = ",";
inline constexpr auto *CP1251 = "cp1251";
inline constexpr auto *DOT = ".";
inline constexpr auto *COMMA = ",";
inline constexpr auto *BACKSLASH = "\\";
inline constexpr auto *QUOTE = "\"";
inline constexpr auto *NEW_LINE = "\n";
inline constexpr auto *SPACE = " ";
inline constexpr auto *WHAT = " \n\r\t\v\0";
inline constexpr auto *ONE = "1";
inline constexpr auto *PERCENT = "%";

}; // namespace string_context_impl_

struct StringLibContext final : vk::not_copyable {
  static constexpr auto STATIC_BUFFER_LENGTH = static_cast<size_t>(1U << 23U);
  static constexpr auto MASK_BUFFER_LENGTH = 256;

  std::array<char, STATIC_BUFFER_LENGTH> static_buf{};
  std::array<char, MASK_BUFFER_LENGTH> mask_buffer{};
  string_buffer buf;
  string_buffer buf_spare;

  int64_t str_replace_count_dummy{};
  double default_similar_text_percent_stub{};

  static StringLibContext &get() noexcept;
};

struct StringLibConstants final : vk::not_copyable {
  const string COLON{string_context_impl_::COLON, static_cast<string::size_type>(std::char_traits<char>::length(string_context_impl_::COLON))};
  const string CP1251{string_context_impl_::CP1251, static_cast<string::size_type>(std::char_traits<char>::length(string_context_impl_::CP1251))};
  const string DOT{string_context_impl_::DOT, static_cast<string::size_type>(std::char_traits<char>::length(string_context_impl_::DOT))};
  const string COMMA{string_context_impl_::COMMA, static_cast<string::size_type>(std::char_traits<char>::length(string_context_impl_::COMMA))};
  const string BACKSLASH{string_context_impl_::BACKSLASH, static_cast<string::size_type>(std::char_traits<char>::length(string_context_impl_::BACKSLASH))};
  const string QUOTE{string_context_impl_::QUOTE, static_cast<string::size_type>(std::char_traits<char>::length(string_context_impl_::QUOTE))};
  const string NEW_LINE{string_context_impl_::NEW_LINE, static_cast<string::size_type>(std::char_traits<char>::length(string_context_impl_::NEW_LINE))};
  const string SPACE{string_context_impl_::SPACE, static_cast<string::size_type>(std::char_traits<char>::length(string_context_impl_::SPACE))};
  const string WHAT{string_context_impl_::WHAT, static_cast<string::size_type>(std::char_traits<char>::length(string_context_impl_::WHAT))};
  const string ONE{string_context_impl_::ONE, static_cast<string::size_type>(std::char_traits<char>::length(string_context_impl_::ONE))};
  const string PERCENT{string_context_impl_::PERCENT, static_cast<string::size_type>(std::char_traits<char>::length(string_context_impl_::PERCENT))};

  const char lhex_digits[17] = "0123456789abcdef";
  const char uhex_digits[17] = "0123456789ABCDEF";

  static constexpr int64_t ENT_HTML401 = 0;
  static constexpr int64_t ENT_COMPAT = 0;
  static constexpr int64_t ENT_QUOTES = 1;
  static constexpr int64_t ENT_NOQUOTES = 2;

  static constexpr int64_t STR_PAD_LEFT = 0;
  static constexpr int64_t STR_PAD_RIGHT = 1;
  static constexpr int64_t STR_PAD_BOTH = 2;

  static StringLibConstants &get() noexcept;
};
