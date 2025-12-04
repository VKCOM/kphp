// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <concepts>
#include <cstdint>
#include <functional>
#include <optional>
#include <utility>
#include <variant>

#include "common/containers/final_action.h"
#include "runtime-common/core/runtime-core.h"
#include "runtime-light/coroutine/task.h"
#include "runtime-light/coroutine/type-traits.h"
#include "runtime-light/stdlib/diagnostics/logs.h"
#include "runtime-light/stdlib/string/regex-include.h"
#include "runtime-light/stdlib/string/regex-state.h"

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

inline constexpr int64_t PREG_NOLIMIT = -1;

enum class trailing_unmatch : uint8_t { skip, include };

using regex_pcre2_group_names_t = kphp::stl::vector<const char*, kphp::memory::script_allocator>;

struct Info final {
  const string& regex;
  std::string_view subject;
  std::string_view replacement;

  int64_t match_count{};
  uint32_t match_options{PCRE2_NO_UTF_CHECK};

  int64_t replace_count{};
  uint32_t replace_options{PCRE2_SUBSTITUTE_UNKNOWN_UNSET | PCRE2_SUBSTITUTE_UNSET_EMPTY};
  // contains a string after replacements if replace_count > 0, nullopt otherwise
  std::optional<string> opt_replace_result;

  Info() = delete;

  Info(const string& regex_, std::string_view subject_, std::string_view replacement_) noexcept
      : regex(regex_),
        subject(subject_),
        replacement(replacement_) {}
};

struct count_updater {
  int64_t& count;
  Optional<std::variant<std::monostate, std::reference_wrapper<int64_t>>>& opt_count;

  void operator()() const noexcept;
};

struct count_finalizer : vk::final_action<count_updater> {
  int64_t count;

  count_finalizer(Optional<std::variant<std::monostate, std::reference_wrapper<int64_t>>>& opt_count)
      : vk::final_action<count_updater>({.count = count, .opt_count = opt_count}) {}
};

// returns the ending offset of the entire match
PCRE2_SIZE set_matches(const kphp::pcre2::compiled_regex& compiled_regex, const kphp::pcre2::group_names_t& group_names,
                       const pcre2::match_view& match_view, int64_t flags, std::optional<std::reference_wrapper<mixed>> opt_matches,
                       trailing_unmatch last_unmatched_policy) noexcept;

std::pair<string_buffer&, const PCRE2_SIZE> reserve_buffer(std::string_view subject) noexcept;

std::optional<string> make_replace_result(int64_t replace_count, string_buffer& sb, PCRE2_SIZE output_length) noexcept;

template<std::invocable<array<string>> F>
coro::task<bool> replace_callback(Info& regex_info, const pcre2::compiled_regex& compiled_regex, const kphp::pcre2::group_names_t& group_names, F callback,
                                  uint64_t limit, int64_t flags) noexcept {
  regex_info.replace_count = 0;

  const auto& regex_state{RegexInstanceState::get()};
  if (!regex_state.match_context) [[unlikely]] {
    co_return false;
  }

  auto [sb, buffer_length] = reserve_buffer(regex_info.subject);

  size_t match_offset{};
  size_t substitute_offset{};
  int64_t replacement_diff_acc{};
  PCRE2_SIZE length_after_replace{buffer_length};
  string subject{regex_info.subject.data(), static_cast<string::size_type>(regex_info.subject.size())};

  for (; limit == std::numeric_limits<uint64_t>::max() || regex_info.replace_count < limit; ++regex_info.replace_count) {
    auto match_view_opt{compiled_regex.match(regex_info.subject, match_offset, regex_info.match_options)};
    if (!match_view_opt.has_value()) [[unlikely]] {
      co_return false;
    }
    auto& match_view{*match_view_opt};
    if (match_view.size() == 0) {
      break;
    }

    mixed mixed_matches;
    auto match_end{set_matches(compiled_regex, group_names, match_view, flags, mixed_matches, trailing_unmatch::skip)};

    auto matches{array<string>{mixed_matches.as_array().size()}};
    for (auto& elem : std::as_const(mixed_matches.as_array())) {
      matches.set_value(elem.get_key(), std::move(elem.get_value().as_string()));
    }

    string replacement{};
    if constexpr (kphp::coro::is_async_function_v<F, array<string>>) {
      replacement = co_await std::invoke(callback, std::move(matches));
    } else {
      replacement = std::invoke(callback, std::move(matches));
    }

    auto replace_one_result = compiled_regex.replace_one({subject.c_str(), subject.size()}, {replacement.c_str(), replacement.size()}, sb, buffer_length,
                                                         substitute_offset, regex_info.replace_options);
    if (!replace_one_result.has_value()) [[unlikely]] {
      co_return false;
    }
    auto& str_after_replace{*replace_one_result};
    length_after_replace = str_after_replace.size();

    match_offset = match_end;
    replacement_diff_acc += static_cast<int64_t>(str_after_replace.size()) - static_cast<int64_t>(subject.size());
    log::debug("match_end={}, replacement_diff_acc={}", match_end, replacement_diff_acc);
    substitute_offset = match_end + replacement_diff_acc;
    subject = std::move(str_after_replace);
  }

  regex_info.opt_replace_result = make_replace_result(regex_info.replace_count, sb, length_after_replace);

  co_return true;
}

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

Optional<string> f$preg_replace(const string& pattern, const string& replacement, const string& subject, int64_t limit = kphp::regex::PREG_NOLIMIT,
                                Optional<std::variant<std::monostate, std::reference_wrapper<int64_t>>> opt_count = {}) noexcept;

Optional<string> f$preg_replace(const mixed& pattern, const string& replacement, const string& subject, int64_t limit = kphp::regex::PREG_NOLIMIT,
                                Optional<std::variant<std::monostate, std::reference_wrapper<int64_t>>> opt_count = {}) noexcept;

Optional<string> f$preg_replace(const mixed& pattern, const mixed& replacement, const string& subject, int64_t limit = kphp::regex::PREG_NOLIMIT,
                                Optional<std::variant<std::monostate, std::reference_wrapper<int64_t>>> opt_count = {}) noexcept;

mixed f$preg_replace(const mixed& pattern, const mixed& replacement, const mixed& subject, int64_t limit = kphp::regex::PREG_NOLIMIT,
                     Optional<std::variant<std::monostate, std::reference_wrapper<int64_t>>> opt_count = {}) noexcept;

template<class T1, class T2, class T3, class = enable_if_t_is_optional<T3>>
auto f$preg_replace(const T1& regex, const T2& replace_val, const T3& subject, int64_t limit = kphp::regex::PREG_NOLIMIT,
                    Optional<std::variant<std::monostate, std::reference_wrapper<int64_t>>> opt_count = {}) noexcept {
  return f$preg_replace(regex, replace_val, subject.val(), limit, opt_count);
}

// === preg_replace_callback ======================================================================

template<std::invocable<array<string>> F>
kphp::coro::task<Optional<string>> f$preg_replace_callback(string pattern, F callback, string subject, int64_t limit = kphp::regex::PREG_NOLIMIT,
                                                           Optional<std::variant<std::monostate, std::reference_wrapper<int64_t>>> opt_count = {},
                                                           int64_t flags = kphp::regex::PREG_NO_FLAGS) noexcept {
  static_assert(std::same_as<kphp::coro::async_function_return_type_t<F, array<string>>, string>);

  auto cf = kphp::regex::count_finalizer{opt_count};

  if (limit < 0 && limit != kphp::regex::PREG_NOLIMIT) [[unlikely]] {
    kphp::log::warning("invalid limit {} in preg_replace_callback", limit);
    co_return Optional<string>{};
  }

  kphp::regex::Info regex_info{pattern, {subject.c_str(), subject.size()}, {}};

  auto* compiled_regex{kphp::pcre2::compiled_regex::compile(regex_info.regex)};
  if (compiled_regex == nullptr) [[unlikely]] {
    co_return Optional<string>{};
  }
  auto group_names{compiled_regex->collect_group_names()};
  if (!co_await kphp::regex::replace_callback(regex_info, *compiled_regex, group_names, std::move(callback),
                                              limit == kphp::regex::PREG_NOLIMIT ? std::numeric_limits<uint64_t>::max() : static_cast<uint64_t>(limit), flags))
      [[unlikely]] {
    co_return Optional<string>{};
  }
  cf.count = regex_info.replace_count;
  co_return regex_info.opt_replace_result.value_or(subject);
}

template<class F>
kphp::coro::task<Optional<string>> f$preg_replace_callback(mixed pattern, F callback, string subject, int64_t limit = kphp::regex::PREG_NOLIMIT,
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
kphp::coro::task<mixed> f$preg_replace_callback(mixed pattern, F callback, mixed subject, int64_t limit = kphp::regex::PREG_NOLIMIT,
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
auto f$preg_replace_callback(T1&& pattern, T2&& callback, T3&& subject, int64_t limit = kphp::regex::PREG_NOLIMIT,
                             Optional<std::variant<std::monostate, std::reference_wrapper<int64_t>>> opt_count = {},
                             int64_t flags = kphp::regex::PREG_NO_FLAGS) noexcept
    -> decltype(f$preg_replace_callback(std::forward<T1>(pattern), std::forward<T2>(callback), std::forward<T3>(subject).val(), limit, opt_count, flags)) {
  co_return co_await f$preg_replace_callback(std::forward<T1>(pattern), std::forward<T2>(callback), std::forward<T3>(subject).val(), limit, opt_count, flags);
}

// === preg_split =================================================================================

Optional<array<mixed>> f$preg_split(const string& pattern, const string& subject, int64_t limit = kphp::regex::PREG_NOLIMIT,
                                    int64_t flags = kphp::regex::PREG_NO_FLAGS) noexcept;

inline Optional<array<mixed>> f$preg_split(const mixed& pattern, const string& subject, int64_t limit = kphp::regex::PREG_NOLIMIT,
                                           int64_t flags = kphp::regex::PREG_NO_FLAGS) noexcept {
  if (!pattern.is_string()) [[unlikely]] {
    kphp::log::warning("preg_split() expects parameter 1 to be string, {} given", pattern.get_type_or_class_name());
    return false;
  }
  return f$preg_split(pattern.as_string(), subject, limit, flags);
}
