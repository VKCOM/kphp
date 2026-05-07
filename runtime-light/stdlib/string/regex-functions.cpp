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

Optional<string> f$preg_replace(const mixed& pattern, const string& replacement, const string& subject, const int64_t limit,
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

Optional<string> f$preg_replace(const mixed& pattern, const mixed& replacement, const string& subject, const int64_t limit,
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

mixed f$preg_replace(const mixed& pattern, const mixed& replacement, const mixed& subject, const int64_t limit,
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
