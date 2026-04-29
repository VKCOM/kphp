// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime-light/stdlib/string/regex-functions.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <utility>
#include <variant>

#include "common/containers/final_action.h"
#include "runtime-common/core/runtime-core.h"
#include "runtime-common/core/std/containers.h"
#include "runtime-light/stdlib/diagnostics/logs.h"

// === preg_match implementation ==================================================================

Optional<int64_t> f$preg_match(const regexp& regex, const string& subject,
                               const Optional<std::variant<std::monostate, std::reference_wrapper<mixed>>>& opt_matches, const int64_t flags,
                               int64_t offset) noexcept {
  if (!kphp::regex::details::preg_match_check(subject, flags, offset)) [[unlikely]] {
    return false;
  }
  return kphp::regex::details::preg_match_base(regex, subject, opt_matches, flags, offset);
}

Optional<int64_t> f$preg_match(const string& pattern, const string& subject,
                               const Optional<std::variant<std::monostate, std::reference_wrapper<mixed>>>& opt_matches, const int64_t flags,
                               int64_t offset) noexcept {
  if (!kphp::regex::details::preg_match_check(subject, flags, offset)) [[unlikely]] {
    return false;
  }
  return kphp::regex::details::preg_match_base(regexp{pattern, subject}, subject, opt_matches, flags, offset);
}

Optional<int64_t> f$preg_match(const mixed& pattern, const string& subject,
                               const Optional<std::variant<std::monostate, std::reference_wrapper<mixed>>>& opt_matches, const int64_t flags,
                               int64_t offset) noexcept {
  return f$preg_match(pattern.to_string(), subject, opt_matches, flags, offset);
}

// === preg_match_all implementation ==============================================================

Optional<int64_t> f$preg_match_all(const regexp& regex, const string& subject,
                                   const Optional<std::variant<std::monostate, std::reference_wrapper<mixed>>>& opt_matches, const int64_t flags,
                                   int64_t offset) noexcept {
  if (!kphp::regex::details::preg_match_all_check(subject, flags, offset)) [[unlikely]] {
    return false;
  }
  return kphp::regex::details::preg_match_all_base(regex, subject, opt_matches, flags, offset);
}

Optional<int64_t> f$preg_match_all(const string& pattern, const string& subject,
                                   const Optional<std::variant<std::monostate, std::reference_wrapper<mixed>>>& opt_matches, const int64_t flags,
                                   int64_t offset) noexcept {
  if (!kphp::regex::details::preg_match_all_check(subject, flags, offset)) [[unlikely]] {
    return false;
  }
  return kphp::regex::details::preg_match_all_base(regexp{pattern, subject}, subject, opt_matches, flags, offset);
}

Optional<int64_t> f$preg_match_all(const mixed& pattern, const string& subject,
                                   const Optional<std::variant<std::monostate, std::reference_wrapper<mixed>>>& opt_matches, const int64_t flags,
                                   const int64_t offset) noexcept {
  return f$preg_match_all(pattern.to_string(), subject, opt_matches, flags, offset);
}

// === preg_replace implementation ================================================================

Optional<string> f$preg_replace(const regexp& regex, const string& replacement, const string& subject, const int64_t limit,
                                Optional<std::variant<std::monostate, std::reference_wrapper<int64_t>>> opt_count) noexcept {
  int64_t count{};
  auto count_finalizer{kphp::regex::details::get_count_finalizer(count, opt_count)};
  const auto& pcre2_replacement{kphp::regex::details::preg_replace_preparing(replacement, limit)};
  if (!pcre2_replacement.has_value()) [[unlikely]] {
    return false;
  }
  return kphp::regex::details::preg_replace_base(regex, subject, pcre2_replacement.value(), limit, count);
}

Optional<string> f$preg_replace(const regexp& regex, const mixed& replacement, const string& subject, int64_t limit,
                                Optional<std::variant<std::monostate, std::reference_wrapper<int64_t>>> opt_count) noexcept {
  int64_t count{};
  auto count_finalizer{kphp::regex::details::get_count_finalizer(count, opt_count)};

  if (replacement.is_array()) {
    kphp::log::warning("parameter mismatch, pattern is a string while replacement is an array");
    return false;
  }

  return f$preg_replace(regex, replacement.to_string(), subject, limit, opt_count);
}

mixed f$preg_replace(const regexp& regex, const string& replacement, const mixed& subject, int64_t limit,
                     const Optional<std::variant<std::monostate, std::reference_wrapper<int64_t>>>& opt_count) noexcept {
  return f$preg_replace(regex, mixed{replacement}, subject, limit, opt_count);
}

mixed f$preg_replace(const regexp& regex, const mixed& replacement, const mixed& subject, int64_t limit,
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

Optional<string> f$preg_replace(const string& pattern, const string& replacement, const string& subject, const int64_t limit,
                                Optional<std::variant<std::monostate, std::reference_wrapper<int64_t>>> opt_count) noexcept {
  int64_t count{};
  auto count_finalizer{kphp::regex::details::get_count_finalizer(count, opt_count)};
  const auto& pcre2_replacement{kphp::regex::details::preg_replace_preparing(replacement, limit)};
  if (!pcre2_replacement.has_value()) [[unlikely]] {
    return false;
  }
  const regexp regex{pattern, subject};
  return kphp::regex::details::preg_replace_base(regex, subject, pcre2_replacement.value(), limit, count);
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

mixed f$preg_replace(const mixed& pattern, const string& replace_val, const mixed& subject, const int64_t limit,
                     const Optional<std::variant<std::monostate, std::reference_wrapper<int64_t>>>& opt_count) noexcept {
  return f$preg_replace(pattern, mixed{replace_val}, subject, limit, opt_count);
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

// === preg_split implementation ==================================================================

Optional<array<mixed>> f$preg_split(const regexp& regex, const string& subject, const int64_t limit, const int64_t flags) noexcept {
  if (!kphp::regex::details::preg_split_check(flags)) {
    return false;
  }
  return kphp::regex::details::preg_split_base(regex, subject, limit, flags);
}

Optional<array<mixed>> f$preg_split(const string& pattern, const string& subject, const int64_t limit, const int64_t flags) noexcept {
  if (!kphp::regex::details::preg_split_check(flags)) {
    return false;
  }
  return kphp::regex::details::preg_split_base(regexp{pattern, subject}, subject, limit, flags);
}

Optional<array<mixed>> f$preg_split(const mixed& pattern, const string& subject, int64_t limit, int64_t flags) noexcept {
  if (!pattern.is_string()) [[unlikely]] {
    kphp::log::warning("preg_split() expects parameter 1 to be string, {} given", pattern.get_type_or_class_name());
    return false;
  }
  return f$preg_split(pattern.as_string(), subject, limit, flags);
}
