// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <array>
#include <cstdint>
#include <string_view>

#include "common/mixin/not_copyable.h"
#include "common/php-functions.h"
#include "runtime-common/core/runtime-core.h"

namespace string_context_impl_ {

using namespace std::literals;

inline constexpr std::string_view COLON_ = ",";
inline constexpr std::string_view CP1251_ = "cp1251";
inline constexpr std::string_view DOT_ = ".";
inline constexpr std::string_view COMMA_ = ",";
inline constexpr std::string_view BACKSLASH_ = "\\";
inline constexpr std::string_view QUOTE_ = "\"";
inline constexpr std::string_view NEWLINE_ = "\n";
inline constexpr std::string_view SPACE_ = " ";
inline constexpr std::string_view WHAT_ = " \n\r\t\v\0"sv;
inline constexpr std::string_view ONE_ = "1";
inline constexpr std::string_view PERCENT_ = "%";

}; // namespace string_context_impl_

struct StringLibContext final : private vk::not_copyable {
  static constexpr int32_t MASK_BUFFER_LENGTH = 256;
  static constexpr int32_t STATIC_BUFFER_LENGTH = 1U << 23U;

  int64_t str_replace_count_dummy{};
  double default_similar_text_percent_stub{};

  // Do not initialize these arrays. Initializing it would zero out the memory,
  // which significantly impacts K2's performance due to the large size of the buffer.
  // The buffer is intended to be used as raw storage, and its contents will be
  // explicitly managed elsewhere in the code.
  std::array<char, STATIC_BUFFER_LENGTH + 1> static_buf; // FIXME: so large static array causes too many page faults in k2 mode
  std::array<char, MASK_BUFFER_LENGTH> mask_buf;

  StringLibContext() noexcept = default;

  static StringLibContext& get() noexcept;
};

struct StringLibConstants final : vk::not_copyable {
  string COLON_STR{string_context_impl_::COLON_.data(), static_cast<string::size_type>(string_context_impl_::COLON_.size())};
  string CP1251_STR{string_context_impl_::CP1251_.data(), static_cast<string::size_type>(string_context_impl_::CP1251_.size())};
  string DOT_STR{string_context_impl_::DOT_.data(), static_cast<string::size_type>(string_context_impl_::DOT_.size())};
  string COMMA_STR{string_context_impl_::COMMA_.data(), static_cast<string::size_type>(string_context_impl_::COMMA_.size())};
  string BACKSLASH_STR{string_context_impl_::BACKSLASH_.data(), static_cast<string::size_type>(string_context_impl_::BACKSLASH_.size())};
  string QUOTE_STR{string_context_impl_::QUOTE_.data(), static_cast<string::size_type>(string_context_impl_::QUOTE_.size())};
  string NEWLINE_STR{string_context_impl_::NEWLINE_.data(), static_cast<string::size_type>(string_context_impl_::NEWLINE_.size())};
  string SPACE_STR{string_context_impl_::SPACE_.data(), static_cast<string::size_type>(string_context_impl_::SPACE_.size())};
  string WHAT_STR{string_context_impl_::WHAT_.data(), static_cast<string::size_type>(string_context_impl_::WHAT_.size())};
  string ONE_STR{string_context_impl_::ONE_.data(), static_cast<string::size_type>(string_context_impl_::ONE_.size())};
  string PERCENT_STR{string_context_impl_::PERCENT_.data(), static_cast<string::size_type>(string_context_impl_::PERCENT_.size())};

  const char lhex_digits[17] = "0123456789abcdef";
  const char uhex_digits[17] = "0123456789ABCDEF";

  static constexpr int64_t ENT_HTML401 = 0;
  static constexpr int64_t ENT_COMPAT = 0;
  static constexpr int64_t ENT_QUOTES = 1;
  static constexpr int64_t ENT_NOQUOTES = 2;

  static constexpr int64_t STR_PAD_LEFT = 0;
  static constexpr int64_t STR_PAD_RIGHT = 1;
  static constexpr int64_t STR_PAD_BOTH = 2;

  StringLibConstants() noexcept {
    COLON_STR.set_reference_counter_to(ExtraRefCnt::for_global_const);
    CP1251_STR.set_reference_counter_to(ExtraRefCnt::for_global_const);
    DOT_STR.set_reference_counter_to(ExtraRefCnt::for_global_const);
    COMMA_STR.set_reference_counter_to(ExtraRefCnt::for_global_const);
    BACKSLASH_STR.set_reference_counter_to(ExtraRefCnt::for_global_const);
    QUOTE_STR.set_reference_counter_to(ExtraRefCnt::for_global_const);
    NEWLINE_STR.set_reference_counter_to(ExtraRefCnt::for_global_const);
    SPACE_STR.set_reference_counter_to(ExtraRefCnt::for_global_const);
    WHAT_STR.set_reference_counter_to(ExtraRefCnt::for_global_const);
    ONE_STR.set_reference_counter_to(ExtraRefCnt::for_global_const);
    PERCENT_STR.set_reference_counter_to(ExtraRefCnt::for_global_const);
  }

  static const StringLibConstants& get() noexcept;
};
