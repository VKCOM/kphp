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

inline constexpr int64_t PREG_REPLACE_NOLIMIT = -1;

using regexp = string;

Optional<int64_t> f$preg_match(const string &pattern, const string &subject, mixed &matches = RegexInstanceState::get().default_matches,
                               int64_t flags = PREG_NO_FLAGS, int64_t offset = 0) noexcept;

Optional<int64_t> f$preg_match_all(const string &pattern, const string &subject, mixed &matches = RegexInstanceState::get().default_matches,
                                   int64_t flags = PREG_NO_FLAGS, int64_t offset = 0) noexcept;

/*
 * PHP's implementation of preg_replace doesn't replace some errors. For example, consider replacement containing
 * back reference $123. It cannot be found in the pattern, but PHP doesn't report it as error, it just treats such
 * back reference as an empty string. Our implementation warns user about such error and returns null.
 */
Optional<string> f$preg_replace(const string &pattern, const string &replacement, const string &subject, int64_t limit = PREG_REPLACE_NOLIMIT,
                                int64_t &count = RegexInstanceState::get().default_preg_replace_count) noexcept;

Optional<string> f$preg_replace(const mixed &pattern, const string &replacement, const string &subject, int64_t limit = PREG_REPLACE_NOLIMIT,
                                int64_t &count = RegexInstanceState::get().default_preg_replace_count) noexcept;

Optional<string> f$preg_replace(const mixed &pattern, const mixed &replacement, const string &subject, int64_t limit = PREG_REPLACE_NOLIMIT,
                                int64_t &count = RegexInstanceState::get().default_preg_replace_count) noexcept;

mixed f$preg_replace(const mixed &pattern, const mixed &replacement, const mixed &subject, int64_t limit = PREG_REPLACE_NOLIMIT,
                     int64_t &count = RegexInstanceState::get().default_preg_replace_count) noexcept;

mixed f$preg_replace(const mixed &pattern, const mixed &replacement, const Optional<string> &subject, int64_t limit = PREG_REPLACE_NOLIMIT,
                     int64_t &count = RegexInstanceState::get().default_preg_replace_count) noexcept;

template<class T1, class T2, class T3, class = enable_if_t_is_optional<T3>>
auto f$preg_replace_callback(const T1 &regex, const T2 &replace_val, const T3 &subject, int64_t limit = -1,
                             int64_t &replace_count = RegexInstanceState::get().default_preg_replace_count) {
  return f$preg_replace_callback(regex, replace_val, subject.val(), limit, replace_count);
}

template<class T>
Optional<string> f$preg_replace_callback(const regexp &, const T &, const string &, int64_t = -1,
                                         int64_t & = RegexInstanceState::get().default_preg_replace_count) {
  php_critical_error("call to unsupported function");
}

template<class T>
mixed f$preg_replace_callback(const regexp &, const T &, const mixed &, int64_t = -1, int64_t & = RegexInstanceState::get().default_preg_replace_count) {
  php_critical_error("call to unsupported function");
}

template<class T, class T2>
auto f$preg_replace_callback(const string &regex, const T &replace_val, const T2 &subject, int64_t limit = -1,
                             int64_t &replace_count = RegexInstanceState::get().default_preg_replace_count) {
  return f$preg_replace_callback(regexp(regex), replace_val, subject, limit, replace_count);
}

template<class T>
Optional<string> f$preg_replace_callback(const mixed &, const T &, const string &, int64_t = -1,
                                         int64_t & = RegexInstanceState::get().default_preg_replace_count) {
  php_critical_error("call to unsupported function");
}

template<class T>
mixed f$preg_replace_callback(const mixed &, const T &, const mixed &, int64_t = -1, int64_t & = RegexInstanceState::get().default_preg_replace_count) {
  php_critical_error("call to unsupported function");
}

inline Optional<array<mixed>> f$preg_split(const string &, const string &, int64_t = -1, int64_t = 0) {
  php_critical_error("call to unsupported function");
}

inline Optional<array<mixed>> f$preg_split(const mixed &, const string &, int64_t = -1, int64_t = 0) {
  php_critical_error("call to unsupported function");
}
