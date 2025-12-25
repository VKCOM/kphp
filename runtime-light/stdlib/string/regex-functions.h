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

namespace kphp::regex::details {

enum class trailing_unmatch : uint8_t { skip, include };

using pcre2_group_names_t = kphp::stl::vector<const char*, kphp::memory::script_allocator>;

template<bool is_offset_capture, bool is_unmatched_as_null>
using dumped_match_t = std::conditional_t<is_offset_capture, array<mixed>, std::conditional_t<is_unmatched_as_null, mixed, string>>;
template<bool is_offset_capture, bool is_unmatched_as_null>
using dumped_matches_t = array<dumped_match_t<is_offset_capture, is_unmatched_as_null>>;

struct pcre2_error {
  int32_t code{};
};

class match_view {
public:
  match_view(std::string_view subject, const PCRE2_SIZE* ovector, size_t num_groups) noexcept
      : m_subject_data{subject},
        m_ovector_ptr{ovector},
        m_num_groups{num_groups} {}

  int32_t size() const noexcept {
    return m_num_groups;
  }

  std::optional<std::string_view> get_group(size_t i) const noexcept;

private:
  std::string_view m_subject_data;
  const PCRE2_SIZE* m_ovector_ptr;
  size_t m_num_groups;
};

struct Info final {
  const string& regex;
  std::string_view subject;
  std::string_view replacement;

  // PCRE compile options of the regex
  uint32_t compile_options{};
  // number of groups including entire match
  uint32_t capture_count{};
  // compiled regex
  pcre2_code_8* regex_code{nullptr};

  // vector of group names
  details::pcre2_group_names_t group_names;

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

class matcher {
public:
  matcher(const Info& info, size_t match_from) noexcept;

  std::expected<std::optional<details::match_view>, details::pcre2_error> next() noexcept;

private:
  const Info& m_regex_info;
  uint64_t m_match_options{};
  PCRE2_SIZE m_current_offset{};
  pcre2_match_data_8* m_match_data{nullptr};
};

std::pair<string_buffer&, const PCRE2_SIZE> reserve_buffer(std::string_view subject) noexcept;

} // namespace kphp::regex::details

namespace std {

template<>
struct formatter<kphp::regex::details::pcre2_error> {
  static constexpr size_t ERROR_BUFFER_LENGTH{256};

  template<typename ParseContext>
  constexpr auto parse(ParseContext& ctx) const noexcept {
    return ctx.begin();
  }

  template<typename FmtContext>
  auto format(kphp::regex::details::pcre2_error error, FmtContext& ctx) const noexcept {
    std::array<char, ERROR_BUFFER_LENGTH> buffer{};
    auto ret_code{pcre2_get_error_message_8(error.code, reinterpret_cast<PCRE2_UCHAR8*>(buffer.data()), buffer.size())};
    if (ret_code < 0) [[unlikely]] {
      switch (ret_code) {
      case PCRE2_ERROR_BADDATA:
        return format_to(ctx.out(), "unknown error ({})", error.code);
      case PCRE2_ERROR_NOMEMORY:
        return format_to(ctx.out(), "[truncated] {}", buffer.data());
      default:
        kphp::log::error("unsupported regex error code: {}", ret_code);
      }
    }
    return format_to(ctx.out(), "{}", buffer.data());
  }
};

} // namespace std

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

bool compile_regex(Info& regex_info) noexcept;

bool collect_group_names(Info& regex_info) noexcept;

std::optional<string> replace_one(const Info& info, std::string_view subject, std::string_view replacement, string_buffer& sb, size_t buffer_length,
                                  size_t substitute_offset) noexcept;

std::optional<array<mixed>> dump_matches(const Info& regex_info, const details::match_view& match, details::trailing_unmatch last_unmatched_policy,
                                         bool is_offset_capture, bool is_unmatched_as_null) noexcept;

template<std::invocable<array<string>> F>
coro::task<bool> replace_callback(Info& regex_info, F callback, uint64_t limit) noexcept {
  regex_info.replace_count = 0;

  const auto& regex_state{RegexInstanceState::get()};
  if (!regex_state.match_context) [[unlikely]] {
    co_return false;
  }

  auto [sb, buffer_length] = details::reserve_buffer(regex_info.subject);

  size_t substitute_offset{};
  int64_t replacement_diff_acc{};
  PCRE2_SIZE length_after_replace{buffer_length};
  string subject{regex_info.subject.data(), static_cast<string::size_type>(regex_info.subject.size())};

  matcher pcre2_matcher{regex_info, {}};
  for (; limit == std::numeric_limits<uint64_t>::max() || regex_info.replace_count < limit; ++regex_info.replace_count) {
    auto expected_opt_match_view{pcre2_matcher.next()};
    if (!expected_opt_match_view.has_value()) [[unlikely]] {
      log::warning("can't replace with callback by pcre2 regex due to match error: {}", expected_opt_match_view.error());
      co_return false;
    }
    auto opt_match_view{*expected_opt_match_view};
    if (!opt_match_view.has_value()) {
      break;
    }
    auto& match_view{*opt_match_view};
    auto opt_entire_pattern_match{match_view.get_group(0)};
    if (!opt_entire_pattern_match.has_value()) [[unlikely]] {
      co_return false;
    }
    auto entire_pattern_match_string_view{*opt_entire_pattern_match};
    const auto match_start_offset{std::distance(regex_info.subject.data(), entire_pattern_match_string_view.data())};
    const auto match_end_offset{match_start_offset + entire_pattern_match_string_view.size()};
    regex_info.match_count = match_view.size();

    auto opt_dumped_matches{dump_matches(regex_info, match_view, details::trailing_unmatch::skip, false, false)};
    if (!opt_dumped_matches.has_value()) [[unlikely]] {
      co_return false;
    }

    const auto& dumped_matches{*opt_dumped_matches};
    auto matches{array<string>{dumped_matches.size()}};
    for (const auto& elem : dumped_matches) {
      matches.set_value(elem.get_key(), elem.get_value().to_string());
    }
    string replacement{};
    if constexpr (kphp::coro::is_async_function_v<F, array<string>>) {
      replacement = co_await std::invoke(callback, std::move(matches));
    } else {
      replacement = std::invoke(callback, std::move(matches));
    }

    auto replace_one_result =
        replace_one(regex_info, {subject.c_str(), subject.size()}, {replacement.c_str(), replacement.size()}, sb, buffer_length, substitute_offset);
    if (!replace_one_result.has_value()) [[unlikely]] {
      co_return false;
    }
    auto& str_after_replace{*replace_one_result};
    length_after_replace = str_after_replace.size();

    replacement_diff_acc += static_cast<int64_t>(str_after_replace.size()) - static_cast<int64_t>(subject.size());
    log::debug("match_end={}, replacement_diff_acc={}", match_end_offset, replacement_diff_acc);
    substitute_offset = match_end_offset + replacement_diff_acc;
    subject = std::move(str_after_replace);
  }

  if (regex_info.replace_count > 0) {
    sb.set_pos(length_after_replace);
    regex_info.opt_replace_result.emplace(sb.str());
  }

  co_return true;
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

  kphp::regex::details::Info regex_info{pattern, {subject.c_str(), subject.size()}, {}};

  if (!kphp::regex::details::valid_regex_flags(flags, kphp::regex::PREG_NO_FLAGS)) [[unlikely]] {
    co_return Optional<string>{};
  }
  if (!kphp::regex::details::compile_regex(regex_info)) [[unlikely]] {
    co_return Optional<string>{};
  }
  if (!kphp::regex::details::collect_group_names(regex_info)) [[unlikely]] {
    co_return Optional<string>{};
  }
  if (!co_await kphp::regex::details::replace_callback(
          regex_info, std::move(callback), limit == kphp::regex::PREG_NOLIMIT ? std::numeric_limits<uint64_t>::max() : static_cast<uint64_t>(limit)))
      [[unlikely]] {
    co_return Optional<string>{};
  }
  count = regex_info.replace_count;
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
