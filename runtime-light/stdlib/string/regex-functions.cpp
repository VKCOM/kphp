// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime-light/stdlib/string/regex-functions.h"

#include <cctype>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <utility>
#include <variant>

#include "runtime-common/core/runtime-core.h"
#include "runtime-light/stdlib/diagnostics/logs.h"

namespace kphp::regex::details {

replace_callback_matcher::replace_callback_matcher(const kphp::regex::regexp& regex, std::string_view subject, int64_t limit) noexcept
    : m_subject{subject},
      m_limit{limit},
      m_opt_re{regex.get_regex()},
      m_unsigned_limit{limit == kphp::regex::PREG_NOLIMIT ? std::numeric_limits<uint64_t>::max() : static_cast<uint64_t>(limit)} {
  if (!m_opt_re.has_value()) [[unlikely]] {
    m_ok = false;
    return;
  }
  const auto& re{m_opt_re->get()};
  m_group_names = collect_group_names(re);

  if (m_limit == 0) {
    return;
  }

  auto& regex_state{RegexInstanceState::get()};
  if (!regex_state.match_context) [[unlikely]] {
    m_ok = false;
    return;
  }

  m_matcher.emplace(re, m_subject, 0, regex_state.match_context, regex_state.match_data, regex.match_options);
}

bool replace_callback_matcher::is_ok() const noexcept {
  return m_ok;
}

int64_t replace_callback_matcher::replace_count() const noexcept {
  return m_replace_count;
}

Optional<array<string>> replace_callback_matcher::next() noexcept {
  if (!is_ok() || !m_matcher.has_value() || m_replace_count >= m_unsigned_limit) {
    return {};
  }

  auto expected_opt_match_view{m_matcher->next()};
  if (!expected_opt_match_view.has_value()) [[unlikely]] {
    kphp::log::warning("can't replace with callback by pcre2 regex due to match error: {}", expected_opt_match_view.error());
    m_ok = false;
    return {};
  }

  auto opt_match_view{*expected_opt_match_view};
  if (!opt_match_view.has_value()) {
    return {};
  }

  auto& match_view{*opt_match_view};
  auto& re{m_opt_re->get()};

  m_output_str.append(std::next(m_subject.data(), m_last_pos), match_view.match_start() - m_last_pos);
  m_last_pos = match_view.match_end();

  // retrieve the named groups count
  uint32_t named_groups_count{re.name_count()};

  array<string> matches{array_size{static_cast<int64_t>(match_view.size() + named_groups_count), named_groups_count == 0}};
  for (auto [key, value] : kphp::regex::details::match_results_wrapper{match_view, m_group_names, re.capture_count(), re.name_count(),
                                                                       kphp::regex::details::trailing_unmatch::skip, false, false}) {
    matches.set_value(key, value.to_string());
  }

  return matches;
}

void replace_callback_matcher::apply(const string& replacement) noexcept {
  m_output_str.append(replacement);
  ++m_replace_count;
}

Optional<string> replace_callback_matcher::finish() noexcept {
  if (!is_ok()) {
    return {};
  }

  m_output_str.append(std::next(m_subject.data(), m_last_pos), m_subject.size() - m_last_pos);
  return m_output_str;
}

} // namespace kphp::regex::details

// === preg_replace implementation ================================================================

mixed f$preg_replace(const kphp::regex::regexp& regex, const mixed& replacement, const mixed& subject, int64_t limit,
                     Optional<std::variant<std::monostate, std::reference_wrapper<int64_t>>> opt_count) noexcept {
  int64_t count{};
  auto count_finalizer{kphp::regex::details::get_count_finalizer(count, opt_count)};

  if (replacement.is_object()) [[unlikely]] {
    kphp::log::warning("invalid replacement: object could not be converted to string");
    return {};
  }
  if (subject.is_object()) [[unlikely]] {
    kphp::log::warning("invalid subject: object could not be converted to string");
    return {};
  }

  if (!subject.is_array()) {
    return f$preg_replace(regex, replacement, subject.to_string(), limit, count);
  }

  const auto& subject_arr{subject.as_array()};
  array<mixed> result{subject_arr.size()};
  for (const auto& it : subject_arr) {
    int64_t replace_one_count{};
    if (Optional replace_result{f$preg_replace(regex, replacement, it.get_value().to_string(), limit, replace_one_count)}; replace_result.has_value())
        [[likely]] {
      count += replace_one_count;
      result.set_value(it.get_key(), std::move(replace_result.val()));
    } else {
      count = 0;
      return {};
    }
  }

  return std::move(result);
}

Optional<string> f$preg_replace(const mixed& pattern, const string& replacement, const string& subject, int64_t limit,
                                Optional<std::variant<std::monostate, std::reference_wrapper<int64_t>>> opt_count) noexcept {
  int64_t count{};
  auto count_finalizer{kphp::regex::details::get_count_finalizer(count, opt_count)};

  if (pattern.is_object()) [[unlikely]] {
    kphp::log::warning("invalid pattern: object could not be converted to string");
    return {};
  }

  if (!pattern.is_array()) {
    return f$preg_replace(pattern.to_string(), replacement, subject, limit, count);
  }

  string result{subject};
  const auto& pattern_arr{pattern.as_array()};
  for (const auto& it : pattern_arr) {
    int64_t replace_one_count{};
    if (Optional replace_result{f$preg_replace(it.get_value().to_string(), replacement, result, limit, replace_one_count)}; replace_result.has_value())
        [[likely]] {
      count += replace_one_count;
      result = std::move(replace_result.val());
    } else {
      count = 0;
      return {};
    }
  }

  return result;
}

Optional<string> f$preg_replace(const mixed& pattern, const mixed& replacement, const string& subject, int64_t limit,
                                Optional<std::variant<std::monostate, std::reference_wrapper<int64_t>>> opt_count) noexcept {
  int64_t count{};
  auto count_finalizer{kphp::regex::details::get_count_finalizer(count, opt_count)};

  if (pattern.is_object()) [[unlikely]] {
    kphp::log::warning("invalid pattern: object could not be converted to string");
    return {};
  }
  if (replacement.is_object()) [[unlikely]] {
    kphp::log::warning("invalid replacement: object could not be converted to string");
    return {};
  }

  if (!replacement.is_array()) {
    return f$preg_replace(pattern, replacement.to_string(), subject, limit, count);
  }
  if (!pattern.is_array()) [[unlikely]] {
    kphp::log::warning("parameter mismatch: replacement is an array while pattern is string");
    return {};
  }

  string result{subject};
  const auto& pattern_arr{pattern.as_array()};
  const auto& replacement_arr{replacement.as_array()};
  auto replacement_it{replacement_arr.cbegin()};
  for (const auto& pattern_it : pattern_arr) {
    string replacement_str{};
    if (replacement_it != replacement_arr.cend()) {
      replacement_str = replacement_it.get_value().to_string();
      ++replacement_it;
    }

    int64_t replace_one_count{};
    if (Optional replace_result{f$preg_replace(pattern_it.get_value().to_string(), replacement_str, result, limit, replace_one_count)};
        replace_result.has_value()) [[likely]] {
      count += replace_one_count;
      result = std::move(replace_result.val());
    } else {
      count = 0;
      return {};
    }
  }

  return result;
}

mixed f$preg_replace(const mixed& pattern, const mixed& replacement, const mixed& subject, int64_t limit,
                     Optional<std::variant<std::monostate, std::reference_wrapper<int64_t>>> opt_count) noexcept {
  int64_t count{};
  auto count_finalizer{kphp::regex::details::get_count_finalizer(count, opt_count)};

  if (pattern.is_object()) [[unlikely]] {
    kphp::log::warning("invalid pattern: object could not be converted to string");
    return {};
  }
  if (replacement.is_object()) [[unlikely]] {
    kphp::log::warning("invalid replacement: object could not be converted to string");
    return {};
  }
  if (subject.is_object()) [[unlikely]] {
    kphp::log::warning("invalid subject: object could not be converted to string");
    return {};
  }

  if (!subject.is_array()) {
    return f$preg_replace(pattern, replacement, subject.to_string(), limit, count);
  }

  const auto& subject_arr{subject.as_array()};
  array<mixed> result{subject_arr.size()};
  for (const auto& it : subject_arr) {
    int64_t replace_one_count{};
    if (Optional replace_result{f$preg_replace(pattern, replacement, it.get_value().to_string(), limit, replace_one_count)}; replace_result.has_value())
        [[likely]] {
      count += replace_one_count;
      result.set_value(it.get_key(), std::move(replace_result.val()));
    } else {
      count = 0;
      return {};
    }
  }

  return std::move(result);
}
