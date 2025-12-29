// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <concepts>
#include <cstdint>
#include <functional>
#include <optional>
#include <type_traits>
#include <utility>
#include <variant>

#include "common/containers/final_action.h"
#include "runtime-common/core/runtime-core.h"
#include "runtime-light/coroutine/task.h"
#include "runtime-light/coroutine/type-traits.h"
#include "runtime-light/stdlib/diagnostics/logs.h"
#include "runtime-light/stdlib/string/regex-include.h"
#include "runtime-light/stdlib/string/regex-state.h"

namespace kphp::regex::details {

enum class trailing_unmatch : uint8_t { skip, include };

using pcre2_group_names_t = kphp::stl::vector<const char*, kphp::memory::script_allocator>;

struct Info final {
  const string& regex;
  const string& subject;
  std::string_view replacement;

  // PCRE compile options of the regex
  uint32_t compile_options{};

  int64_t match_count{};
  uint32_t match_options{PCRE2_NO_UTF_CHECK};

  int64_t replace_count{};
  uint32_t replace_options{PCRE2_SUBSTITUTE_UNKNOWN_UNSET | PCRE2_SUBSTITUTE_UNSET_EMPTY};

  Info() = delete;

  Info(const string& regex_, const string& subject_, std::string_view replacement_) noexcept
      : regex(regex_),
        subject(subject_),
        replacement(replacement_) {}
};

struct match_pair {
  mixed key;
  mixed value;
};

class match_results_wrapper {
public:
  match_results_wrapper(const pcre2::match_view& match_view, const pcre2_group_names_t& names, uint32_t capture_count, uint32_t name_count,
                        trailing_unmatch last_unmatched_policy, bool is_offset_capture, bool is_unmatched_as_null) noexcept
      : m_view{match_view},
        m_group_names{names},
        m_capture_count{capture_count},
        m_name_count{name_count},
        m_last_unmatched_policy{last_unmatched_policy},
        m_is_offset_capture{is_offset_capture},
        m_is_unmatched_as_null{is_unmatched_as_null} {}

  uint32_t match_count() const noexcept {
    if (!m_is_unmatched_as_null && m_last_unmatched_policy == trailing_unmatch::skip) {
      return m_view.size();
    }
    return m_capture_count + 1;
  }

  size_t max_potential_size() const noexcept {
    return match_count() + m_name_count;
  }

  uint32_t name_count() const noexcept {
    return m_name_count;
  }

  class iterator {
  public:
    using iterator_category = std::forward_iterator_tag;
    using value_type = match_pair;
    using difference_type = std::ptrdiff_t;
    using pointer = match_pair*;
    using reference = match_pair;

    iterator(const match_results_wrapper& parent, uint32_t group_idx) noexcept
        : m_parent{parent},
          m_group_idx{group_idx} {
      if (m_group_idx < m_parent.m_group_names.size() && m_parent.m_group_names[m_group_idx] != nullptr) {
        m_yield_name = true;
      }
    }

    match_pair operator*() const noexcept;

    iterator& operator++() noexcept {
      if (m_yield_name) {
        m_yield_name = false;
      } else {
        m_group_idx++;

        if (m_group_idx < m_parent.m_group_names.size() && m_parent.m_group_names[m_group_idx] != nullptr) {
          m_yield_name = true;
        }
      }
      return *this;
    }

    bool operator==(const iterator& other) const noexcept {
      return m_group_idx == other.m_group_idx && m_yield_name == other.m_yield_name;
    }
    bool operator!=(const iterator& other) const noexcept {
      return !(*this == other);
    }

  private:
    const match_results_wrapper& m_parent;
    uint32_t m_group_idx;
    bool m_yield_name{false};
  };

  iterator begin() const noexcept {
    return iterator{*this, 0};
  }

  iterator end() const noexcept {
    return iterator{*this, match_count()};
  }

private:
  const pcre2::match_view& m_view;
  const pcre2_group_names_t& m_group_names;
  uint32_t m_capture_count;
  uint32_t m_name_count;
  trailing_unmatch m_last_unmatched_policy;
  bool m_is_offset_capture;
  bool m_is_unmatched_as_null;
};

std::pair<string_buffer&, PCRE2_SIZE> reserve_buffer(std::string_view subject) noexcept;

} // namespace kphp::regex::details

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

namespace details {

template<typename... Args>
requires((std::is_same_v<Args, int64_t> && ...) && sizeof...(Args) > 0)
bool valid_regex_flags(int64_t flags, Args... supported_flags) noexcept {
  const bool valid{(flags & ~(supported_flags | ...)) == kphp::regex::PREG_NO_FLAGS};
  if (!valid) [[unlikely]] {
    kphp::log::warning("invalid flags: {}", flags);
  }
  return valid;
}

std::optional<std::reference_wrapper<const pcre2::regex>> compile_regex(Info& regex_info) noexcept;

pcre2_group_names_t collect_group_names(const pcre2::regex& re) noexcept;

template<std::invocable<array<string>> F>
coro::task<std::optional<string>> replace_callback(Info& regex_info, const pcre2::regex& re, const pcre2_group_names_t& group_names, F callback,
                                                   uint64_t limit) noexcept {
  regex_info.replace_count = 0;

  if (limit == 0) {
    co_return regex_info.subject;
  }

  const auto& regex_state{RegexInstanceState::get()};
  if (!regex_state.match_context) [[unlikely]] {
    co_return std::nullopt;
  }

  size_t last_pos{};
  string output_str{};

  pcre2::matcher pcre2_matcher{re,
                               {regex_info.subject.c_str(), regex_info.subject.size()},
                               {},
                               regex_state.match_context.get(),
                               *regex_state.regex_pcre2_match_data,
                               regex_info.match_options};
  while (regex_info.replace_count < limit) {
    auto expected_opt_match_view{pcre2_matcher.next()};

    if (!expected_opt_match_view.has_value()) [[unlikely]] {
      log::warning("can't replace with callback by pcre2 regex due to match error: {}", expected_opt_match_view.error());
      co_return std::nullopt;
    }
    auto opt_match_view{*expected_opt_match_view};
    if (!opt_match_view.has_value()) {
      break;
    }

    auto& match_view{*opt_match_view};

    output_str.append(std::next(regex_info.subject.c_str(), last_pos), match_view.match_start() - last_pos);

    last_pos = match_view.match_end();

    // retrieve the named groups count
    uint32_t named_groups_count{re.name_count()};

    array<string> matches{array_size{static_cast<int64_t>(match_view.size() + named_groups_count), named_groups_count == 0}};
    for (auto [key, value] : match_results_wrapper{match_view, group_names, re.capture_count(), re.name_count(), trailing_unmatch::skip, false, false}) {
      matches.set_value(key, value.to_string());
    }
    string replacement{};
    if constexpr (kphp::coro::is_async_function_v<F, array<string>>) {
      replacement = co_await std::invoke(callback, std::move(matches));
    } else {
      replacement = std::invoke(callback, std::move(matches));
    }

    output_str.append(replacement);

    ++regex_info.replace_count;
  }

  output_str.append(std::next(regex_info.subject.c_str(), last_pos), regex_info.subject.size() - last_pos);

  co_return output_str;
}

} // namespace details

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

  int64_t count{};
  vk::final_action count_finalizer{[&count, &opt_count]() noexcept {
    if (opt_count.has_value()) {
      kphp::log::assertion(std::holds_alternative<std::reference_wrapper<int64_t>>(opt_count.val()));
      auto& inner_ref{std::get<std::reference_wrapper<int64_t>>(opt_count.val()).get()};
      inner_ref = count;
    }
  }};

  if (limit < 0 && limit != kphp::regex::PREG_NOLIMIT) [[unlikely]] {
    kphp::log::warning("invalid limit {} in preg_replace_callback", limit);
    co_return Optional<string>{};
  }

  kphp::regex::details::Info regex_info{pattern, subject, {}};

  if (!kphp::regex::details::valid_regex_flags(flags, kphp::regex::PREG_NO_FLAGS)) [[unlikely]] {
    co_return Optional<string>{};
  }
  auto opt_re{compile_regex(regex_info)};
  if (!opt_re.has_value()) [[unlikely]] {
    co_return Optional<string>{};
  }
  const auto& re{*opt_re};
  auto group_names{kphp::regex::details::collect_group_names(re)};
  auto opt_replace_result{co_await kphp::regex::details::replace_callback(regex_info, re, group_names, std::move(callback),
                                                                          limit == kphp::regex::PREG_NOLIMIT ? std::numeric_limits<uint64_t>::max()
                                                                                                             : static_cast<uint64_t>(limit))};
  if (!opt_replace_result.has_value()) [[unlikely]] {
    co_return Optional<string>{};
  }
  count = regex_info.replace_count;
  co_return *opt_replace_result;
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
