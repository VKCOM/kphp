// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cstdint>

#include "runtime-common/core/runtime-core.h"
#include "runtime-light/stdlib/string/regex-state.h"

inline constexpr int64_t PREG_NO_ERROR = 0;
inline constexpr int64_t PREG_INTERNAL_ERROR = 1;
inline constexpr int64_t PREG_BACKTRACK_LIMIT_ERROR = 2;
inline constexpr int64_t PREG_RECURSION_LIMIT = 3;
inline constexpr int64_t PREG_BAD_UTF8_ERROR = 4;
inline constexpr int64_t PREG_BAD_UTF8_OFFSET_ERROR = 5;

inline constexpr int64_t PREG_NO_FLAGS = 0;
inline constexpr auto PREG_PATTERN_ORDER = static_cast<int64_t>(1U << 0U);
inline constexpr auto PREG_SET_ORDER = static_cast<int64_t>(1U << 1U);
inline constexpr auto PREG_OFFSET_CAPTURE = static_cast<int64_t>(1U << 2U);
inline constexpr auto PREG_SPLIT_NO_EMPTY = static_cast<int64_t>(1U << 3U);
inline constexpr auto PREG_SPLIT_DELIM_CAPTURE = static_cast<int64_t>(1U << 4U);
inline constexpr auto PREG_SPLIT_OFFSET_CAPTURE = static_cast<int64_t>(1U << 5U);
inline constexpr auto PREG_UNMATCHED_AS_NULL = static_cast<int64_t>(1U << 6U);

namespace regex_impl_ {

inline constexpr int64_t PREG_REPLACE_NOLIMIT = -1;

} // namespace regex_impl_

Optional<int64_t> f$preg_match(const string &pattern, const string &subject, mixed &matches = RegexInstanceState::get().default_matches,
                               int64_t flags = PREG_NO_FLAGS, int64_t offset = 0) noexcept;

Optional<int64_t> f$preg_match_all(const string &pattern, const string &subject, mixed &matches = RegexInstanceState::get().default_matches,
                                   int64_t flags = PREG_NO_FLAGS, int64_t offset = 0) noexcept;

Optional<string> f$preg_replace(const string &pattern, const string &replacement, const string &subject, int64_t limit = regex_impl_::PREG_REPLACE_NOLIMIT,
                                int64_t &count = RegexInstanceState::get().default_preg_replace_count) noexcept;
