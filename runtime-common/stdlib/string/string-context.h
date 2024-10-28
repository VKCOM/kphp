// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <array>
#include <cstdint>
#include <string>

#include "common/mixin/not_copyable.h"
#include "runtime-common/core/runtime-core.h"

namespace string_context_impl_ {

inline constexpr auto *COLON_ = ",";
inline constexpr auto *CP1251_ = "cp1251";
inline constexpr auto *DOT_ = ".";
inline constexpr auto *COMMA_ = ",";
inline constexpr auto *BACKSLASH_ = "\\";
inline constexpr auto *QUOTE_ = "\"";
inline constexpr auto *NEWLINE_ = "\n";
inline constexpr auto *SPACE_ = " ";
inline constexpr auto *WHAT_ = " \n\r\t\v\0";
inline constexpr auto *ONE_ = "1";
inline constexpr auto *PERCENT_ = "%";

}; // namespace string_context_impl_

class StringLibContext final : vk::not_copyable {
  static constexpr int32_t MASK_BUFFER_LENGTH = 256;

public:
  static constexpr int32_t STATIC_BUFFER_LENGTH = 1U << 23U;

  std::array<char, STATIC_BUFFER_LENGTH + 1> static_buf{};
  std::array<char, MASK_BUFFER_LENGTH> mask_buffer{};

  int64_t str_replace_count_dummy{};
  double default_similar_text_percent_stub{};

  static StringLibContext &get() noexcept;
};

struct StringLibConstants final : vk::not_copyable {
  const string COLON_STR{string_context_impl_::COLON_, static_cast<string::size_type>(std::char_traits<char>::length(string_context_impl_::COLON_))};
  const string CP1251_STR{string_context_impl_::CP1251_, static_cast<string::size_type>(std::char_traits<char>::length(string_context_impl_::CP1251_))};
  const string DOT_STR{string_context_impl_::DOT_, static_cast<string::size_type>(std::char_traits<char>::length(string_context_impl_::DOT_))};
  const string COMMA_STR{string_context_impl_::COMMA_, static_cast<string::size_type>(std::char_traits<char>::length(string_context_impl_::COMMA_))};
  const string BACKSLASH_STR{string_context_impl_::BACKSLASH_,
                             static_cast<string::size_type>(std::char_traits<char>::length(string_context_impl_::BACKSLASH_))};
  const string QUOTE_STR{string_context_impl_::QUOTE_, static_cast<string::size_type>(std::char_traits<char>::length(string_context_impl_::QUOTE_))};
  const string NEWLINE_STR{string_context_impl_::NEWLINE_, static_cast<string::size_type>(std::char_traits<char>::length(string_context_impl_::NEWLINE_))};
  const string SPACE_STR{string_context_impl_::SPACE_, static_cast<string::size_type>(std::char_traits<char>::length(string_context_impl_::SPACE_))};
  // +1 here to since char_traits<char>::length doesn't count '\0' at the end
  const string WHAT_STR{string_context_impl_::WHAT_, static_cast<string::size_type>(std::char_traits<char>::length(string_context_impl_::WHAT_)) + 1};
  const string ONE_STR{string_context_impl_::ONE_, static_cast<string::size_type>(std::char_traits<char>::length(string_context_impl_::ONE_))};
  const string PERCENT_STR{string_context_impl_::PERCENT_, static_cast<string::size_type>(std::char_traits<char>::length(string_context_impl_::PERCENT_))};

  const char lhex_digits[17] = "0123456789abcdef";
  const char uhex_digits[17] = "0123456789ABCDEF";

  static constexpr int64_t ENT_HTML401 = 0;
  static constexpr int64_t ENT_COMPAT = 0;
  static constexpr int64_t ENT_QUOTES = 1;
  static constexpr int64_t ENT_NOQUOTES = 2;

  static constexpr int64_t STR_PAD_LEFT = 0;
  static constexpr int64_t STR_PAD_RIGHT = 1;
  static constexpr int64_t STR_PAD_BOTH = 2;

  static const StringLibConstants &get() noexcept;
};
