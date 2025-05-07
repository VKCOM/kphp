// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <concepts>
#include <cstdint>
#include <functional>
#include <utility>
#include <variant>

#include "common/containers/final_action.h"
#include "runtime-common/core/runtime-core.h"
#include "runtime-light/coroutine/task.h"
#include "runtime-light/coroutine/type-traits.h"
#include "runtime-light/utils/logs.h"

namespace kphp::regex {

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

} // namespace kphp::regex

namespace regex_impl_ {

inline bool valid_preg_replace_mixed(const mixed& param) noexcept {
  if (!param.is_array() && !param.is_string()) [[unlikely]] {
    kphp::log::warning("invalid parameter: expected to be string or array");
    return false;
  }
  return true;
}

} // namespace regex_impl_

using regexp = string;

// === preg_match =================================================================================

Optional<int64_t> f$preg_match(const string& pattern, const string& subject,
                               Optional<std::variant<std::monostate, std::reference_wrapper<mixed>>> opt_matches = {},
                               int64_t flags = kphp::regex::PREG_NO_FLAGS, int64_t offset = 0) noexcept;

// === preg_match_all =============================================================================

Optional<int64_t> f$preg_match_all(const string& pattern, const string& subject,
                                   Optional<std::variant<std::monostate, std::reference_wrapper<mixed>>> opt_matches = {},
                                   int64_t flags = kphp::regex::PREG_NO_FLAGS, int64_t offset = 0) noexcept;

// === preg_replace ===============================================================================

Optional<string> f$preg_replace(const string& pattern, const string& replacement, const string& subject, int64_t limit = kphp::regex::PREG_REPLACE_NOLIMIT,
                                Optional<std::variant<std::monostate, std::reference_wrapper<int64_t>>> opt_count = {}) noexcept;

Optional<string> f$preg_replace(const mixed& pattern, const string& replacement, const string& subject, int64_t limit = kphp::regex::PREG_REPLACE_NOLIMIT,
                                Optional<std::variant<std::monostate, std::reference_wrapper<int64_t>>> opt_count = {}) noexcept;

Optional<string> f$preg_replace(const mixed& pattern, const mixed& replacement, const string& subject, int64_t limit = kphp::regex::PREG_REPLACE_NOLIMIT,
                                Optional<std::variant<std::monostate, std::reference_wrapper<int64_t>>> opt_count = {}) noexcept;

mixed f$preg_replace(const mixed& pattern, const mixed& replacement, const mixed& subject, int64_t limit = kphp::regex::PREG_REPLACE_NOLIMIT,
                     Optional<std::variant<std::monostate, std::reference_wrapper<int64_t>>> opt_count = {}) noexcept;

template<class T1, class T2, class T3, class = enable_if_t_is_optional<T3>>
auto f$preg_replace(const T1& regex, const T2& replace_val, const T3& subject, int64_t limit = kphp::regex::PREG_REPLACE_NOLIMIT,
                    Optional<std::variant<std::monostate, std::reference_wrapper<int64_t>>> opt_count = {}) noexcept {
  return f$preg_replace(regex, replace_val, subject.val(), limit, opt_count);
}

// === preg_replace_callback ======================================================================

template<std::invocable<array<string>> F>
kphp::coro::task<Optional<string>> f$preg_replace_callback(string pattern, F callback, string subject, int64_t limit = kphp::regex::PREG_REPLACE_NOLIMIT,
                                                           Optional<std::variant<std::monostate, std::reference_wrapper<int64_t>>> opt_count = {},
                                                           int64_t flags = kphp::regex::PREG_NO_FLAGS) noexcept {
  static_assert(std::same_as<kphp::coro::async_function_return_type_t<F, array<string>>, string>);
  // the performance of this function can be enhanced:
  // 1. don't use public f$preg_match and f$preg_replace;
  // 2. use match_regex and replace_regex directly;
  // 3. reuse match_data from match_regex in replace_regex.
  array<string> matches{};
  { // fill matches array or early return
    mixed mixed_matches{};
    const auto match_result{f$preg_match(pattern, subject, mixed_matches, flags, 0)};
    if (!match_result.has_value()) [[unlikely]] {
      co_return Optional<string>{};
    } else if (match_result.val() == 0) { // no matches, so just return the subject
      co_return std::move(subject);
    }

    matches = array<string>{mixed_matches.as_array().size()};
    for (auto& elem : mixed_matches.as_array()) {
      matches.set_value(elem.get_key(), std::move(elem.get_value().as_string()));
    }
  }

  string replacement{};
  if constexpr (kphp::coro::is_async_function_v<F, array<string>>) {
    replacement = co_await std::invoke(callback, std::move(matches));
  } else {
    replacement = std::invoke(callback, std::move(matches));
  }

  co_return f$preg_replace(pattern, replacement, subject, limit, opt_count);
}

template<class F>
kphp::coro::task<Optional<string>> f$preg_replace_callback(mixed pattern, F callback, string subject, int64_t limit = kphp::regex::PREG_REPLACE_NOLIMIT,
                                                           Optional<std::variant<std::monostate, std::reference_wrapper<int64_t>>> opt_count = {},
                                                           int64_t flags = kphp::regex::PREG_NO_FLAGS) noexcept {
  if (!regex_impl_::valid_preg_replace_mixed(pattern)) [[unlikely]] {
    co_return Optional<string>{};
  }

  if (pattern.is_string()) {
    co_return co_await f$preg_replace_callback(std::move(pattern.as_string()), std::move(callback), std::move(subject), limit, opt_count, flags);
  }

  int64_t count{};
  vk::final_action count_finalizer{[&count, &opt_count]() noexcept {
    if (!opt_count.has_value()) {
      return;
    }
    kphp::log::assertion(std::holds_alternative<std::reference_wrapper<int64_t>>(opt_count.val()));
    auto& inner_ref{std::get<std::reference_wrapper<int64_t>>(opt_count.val()).get()};
    inner_ref = count;
  }};

  string result{subject};
  const auto& pattern_arr{pattern.as_array()};
  for (const auto& it : pattern_arr) {
    int64_t replace_one_count{};
    if (auto replace_result{co_await f$preg_replace_callback(it.get_value().to_string(), callback, std::move(result), limit, replace_one_count, flags)};
        replace_result.has_value()) [[likely]] {
      count += replace_one_count;
      result = std::move(replace_result.val());
    } else {
      count = 0;
      co_return Optional<string>{};
    }
  }

  co_return std::move(result);
}

template<class F>
kphp::coro::task<mixed> f$preg_replace_callback(mixed pattern, F callback, mixed subject, int64_t limit = kphp::regex::PREG_REPLACE_NOLIMIT,
                                                Optional<std::variant<std::monostate, std::reference_wrapper<int64_t>>> opt_count = {},
                                                int64_t flags = kphp::regex::PREG_NO_FLAGS) noexcept {
  if (!regex_impl_::valid_preg_replace_mixed(pattern) || !regex_impl_::valid_preg_replace_mixed(subject)) [[unlikely]] {
    co_return mixed{};
  }

  if (subject.is_string()) {
    co_return co_await f$preg_replace_callback(std::move(pattern), std::move(callback), std::move(subject.as_string()), limit, opt_count, flags);
  }

  int64_t count{};
  vk::final_action count_finalizer{[&count, &opt_count]() noexcept {
    if (!opt_count.has_value()) {
      return;
    }
    kphp::log::assertion(std::holds_alternative<std::reference_wrapper<int64_t>>(opt_count.val()));
    auto& inner_ref{std::get<std::reference_wrapper<int64_t>>(opt_count.val()).get()};
    inner_ref = count;
  }};

  const auto& subject_arr{subject.as_array()};
  array<mixed> result{subject_arr.size()};
  for (const auto& it : subject_arr) {
    int64_t replace_one_count{};
    if (auto replace_result{co_await f$preg_replace_callback(pattern, callback, it.get_value().to_string(), limit, replace_one_count, flags)};
        replace_result.has_value()) [[likely]] {
      count += replace_one_count;
      result.set_value(it.get_key(), std::move(replace_result.val()));
    } else {
      count = 0;
      co_return mixed{};
    }
  }

  co_return std::move(result);
}

template<class T1, class T2, class T3, class = enable_if_t_is_optional<T3>>
auto f$preg_replace_callback(T1&& pattern, T2&& callback, T3&& subject, int64_t limit = kphp::regex::PREG_REPLACE_NOLIMIT,
                             Optional<std::variant<std::monostate, std::reference_wrapper<int64_t>>> opt_count = {},
                             int64_t flags = kphp::regex::PREG_NO_FLAGS) noexcept
    -> decltype(f$preg_replace_callback(std::forward<T1>(pattern), std::forward<T2>(callback), std::forward<T3>(subject).val(), limit, opt_count, flags)) {
  co_return co_await f$preg_replace_callback(std::forward<T1>(pattern), std::forward<T2>(callback), std::forward<T3>(subject).val(), limit, opt_count, flags);
}

// === preg_split =================================================================================

inline Optional<array<mixed>> f$preg_split(const string& /*unused*/, const string& /*unused*/, int64_t /*unused*/ = -1, int64_t /*unused*/ = 0) {
  kphp::log::error("call to unsupported function");
}

inline Optional<array<mixed>> f$preg_split(const mixed& /*unused*/, const string& /*unused*/, int64_t /*unused*/ = -1, int64_t /*unused*/ = 0) {
  kphp::log::error("call to unsupported function");
}
