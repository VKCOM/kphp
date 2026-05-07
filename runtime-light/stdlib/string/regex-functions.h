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
#include "runtime-common/core/allocator/script-allocator.h"
#include "runtime-common/core/runtime-core.h"
#include "runtime-common/core/std/containers.h"
#include "runtime-common/stdlib/string/mbstring-functions.h"
#include "runtime-light/coroutine/task.h"
#include "runtime-light/coroutine/type-traits.h"
#include "runtime-light/stdlib/diagnostics/logs.h"
// correctly include PCRE2 lib
#include "runtime-light/stdlib/string/regex-state.h"

namespace kphp::regex {

class regexp final {
private:
  void compile_regex(kphp::regex::details::RegexCoreState& regex_state, string pattern, const string& subject = {}) noexcept {
    if (!should_compile(regex_state, pattern)) {
      return;
    }
    if (pattern.empty()) {
      kphp::log::warning("empty regex");
      return;
    }

    char end_delim{};
    switch (const char start_delim{pattern[0]}; start_delim) {
    case '(': {
      end_delim = ')';
      break;
    }
    case '[': {
      end_delim = ']';
      break;
    }
    case '{': {
      end_delim = '}';
      break;
    }
    case '<': {
      end_delim = '>';
      break;
    }
    case '>':
    case '!' ... '\'':
    case '*' ... '/':
    case ':':
    case ';':
    case '=':
    case '?':
    case '@':
    case '^':
    case '_':
    case '`':
    case '|':
    case '~': {
      end_delim = start_delim;
      break;
    }
    default: {
      kphp::log::warning("wrong regex delimiter {}", start_delim);
      return;
    }
    }

    uint32_t compile_options{};
    std::string_view regex_body{pattern.c_str(), pattern.size()};

    // remove start delimiter
    regex_body.remove_prefix(1);
    // parse compile options and skip all symbols until the end delimiter
    for (; !regex_body.empty() && regex_body.back() != end_delim; regex_body.remove_suffix(1)) {
      // spaces and newlines are ignored
      if (regex_body.back() == ' ' || regex_body.back() == '\n') {
        continue;
      }

      switch (regex_body.back()) {
      case 'i': {
        compile_options |= PCRE2_CASELESS;
        break;
      }
      case 'm': {
        compile_options |= PCRE2_MULTILINE;
        break;
      }
      case 's': {
        compile_options |= PCRE2_DOTALL;
        break;
      }
      case 'x': {
        compile_options |= PCRE2_EXTENDED;
        break;
      }
      case 'A': {
        compile_options |= PCRE2_ANCHORED;
        break;
      }
      case 'D': {
        compile_options |= PCRE2_DOLLAR_ENDONLY;
        break;
      }
      case 'U': {
        compile_options |= PCRE2_UNGREEDY;
        break;
      }
      case 'X': {
        compile_options |= PCRE2_EXTRA_BAD_ESCAPE_IS_LITERAL;
        break;
      }
      case 'J': {
        compile_options |= PCRE2_INFO_JCHANGED;
        break;
      }
      case 'u': {
        compile_options |= PCRE2_UTF | PCRE2_UCP;
        break;
      }
      default: {
        kphp::log::warning("unsupported regex modifier {}", regex_body.back());
        break;
      }
      }
    }

    if (regex_body.empty()) {
      kphp::log::warning("no ending regex delimiter: {}", pattern.c_str());
      return;
    }
    // UTF-8 validation
    if (static_cast<bool>(compile_options & PCRE2_UTF)) {
      if (!mb_UTF8_check(pattern.c_str())) [[unlikely]] {
        kphp::log::warning("invalid UTF-8 pattern: {}", pattern.c_str());
        return;
      }
      if (!mb_UTF8_check(subject.c_str())) [[unlikely]] {
        kphp::log::warning("invalid UTF-8 subject: {}", pattern.c_str());
        return;
      }
    }

    // remove the end delimiter
    regex_body.remove_suffix(1);
    this->compile_options = compile_options;

    // compile pcre2_code
    auto expected_re{kphp::pcre2::regex::compile(regex_body, regex_state.compile_context, this->compile_options)};
    if (!expected_re.has_value()) [[unlikely]] {
      const auto& err{expected_re.error()};
      kphp::log::warning("can't compile pcre2 regex due to error: {}", static_cast<const kphp::pcre2::error&>(err));
      return;
    }

    auto& re{*expected_re};
    // add compiled code to runtime cache
    m_re = regex_state.add_compiled_regex(pattern, this->compile_options, std::move(re))->get().regex_code;
  }

  bool should_compile(const kphp::regex::details::RegexCoreState& regex_state, const string& pattern) noexcept {
    if (!regex_state.compile_context) [[unlikely]] {
      return false;
    }
    if (auto opt_ref{regex_state.get_compiled_regex(pattern)}; opt_ref.has_value()) {
      const auto& [compile_options, regex_code]{opt_ref->get()};
      this->compile_options = compile_options;
      m_re = regex_code;
      return false;
    }
    return true;
  }

  std::optional<std::reference_wrapper<const kphp::pcre2::regex>> m_re;

public:
  regexp() noexcept = default;

  explicit regexp(const string& pattern, const string& subject) noexcept {
    if (!should_compile(RegexImageState::get(), pattern)) {
      return;
    }
    compile_regex(RegexInstanceState::get(), pattern, subject);
  }

  // DO NOT USE. For code-gen purposes only
  void compile_time_init(const string& pattern) noexcept {
    compile_regex(RegexImageState::get_mutable(), pattern);
  }

  auto get_regex() const noexcept {
    return m_re;
  }

  // PCRE compile options of the regex
  uint32_t compile_options{};

  int64_t match_count{};
  uint32_t match_options{PCRE2_NO_UTF_CHECK};

  int64_t replace_count{};
  uint32_t replace_options{PCRE2_SUBSTITUTE_UNKNOWN_UNSET | PCRE2_SUBSTITUTE_UNSET_EMPTY};
};

inline constexpr int64_t PREG_NOLIMIT = -1;
inline constexpr int64_t PREG_NO_FLAGS = 0;
inline constexpr int64_t PREG_NO_ERROR = 0;
inline constexpr int64_t PREG_INTERNAL_ERROR = 1;
inline constexpr int64_t PREG_BACKTRACK_LIMIT_ERROR = 2;
inline constexpr int64_t PREG_RECURSION_LIMIT = 3;
inline constexpr int64_t PREG_BAD_UTF8_ERROR = 4;
inline constexpr int64_t PREG_BAD_UTF8_OFFSET_ERROR = 5;

inline constexpr auto PREG_PATTERN_ORDER = static_cast<int64_t>(1U << 0U);
inline constexpr auto PREG_SET_ORDER = static_cast<int64_t>(1U << 1U);
inline constexpr auto PREG_OFFSET_CAPTURE = static_cast<int64_t>(1U << 2U);
inline constexpr auto PREG_SPLIT_NO_EMPTY = static_cast<int64_t>(1U << 3U);
inline constexpr auto PREG_SPLIT_DELIM_CAPTURE = static_cast<int64_t>(1U << 4U);
inline constexpr auto PREG_SPLIT_OFFSET_CAPTURE = static_cast<int64_t>(1U << 5U);
inline constexpr auto PREG_UNMATCHED_AS_NULL = static_cast<int64_t>(1U << 6U);

namespace details {

enum class trailing_unmatch : uint8_t { skip, include };

using backref = std::string_view;
using replacement_term = std::variant<char, backref>;

template<typename... Args>
requires((std::is_same_v<Args, int64_t> && ...) && sizeof...(Args) > 0)
bool valid_regex_flags(int64_t flags, Args... supported_flags) noexcept {
  const bool valid{(flags & ~(supported_flags | ...)) == kphp::regex::PREG_NO_FLAGS};
  if (!valid) [[unlikely]] {
    kphp::log::warning("invalid flags: {}", flags);
  }
  return valid;
}

inline bool correct_offset(int64_t& offset, const std::string_view subject) noexcept {
  if (offset < 0) [[unlikely]] {
    offset += subject.size();
    if (offset < 0) [[unlikely]] {
      offset = 0;
      return true;
    }
  }
  return offset <= subject.size();
}

inline std::optional<kphp::regex::details::backref> try_get_backref(std::string_view preg_replacement) noexcept {
  if (preg_replacement.empty() || !std::isdigit(preg_replacement[0])) {
    return std::nullopt;
  }

  if (preg_replacement.size() == 1 || !std::isdigit(preg_replacement[1])) {
    return kphp::regex::details::backref{preg_replacement.substr(0, 1)};
  }

  return kphp::regex::details::backref{preg_replacement.substr(0, 2)};
}

inline std::optional<string> replace_regex(const regexp& regex, const string& subject, const std::optional<string>& replacement, const kphp::pcre2::regex& re,
                                           const uint64_t limit, int64_t& replace_count) noexcept {
  replace_count = 0;

  if (limit == 0) {
    return subject;
  }

  auto& regex_state{RegexInstanceState::get()};
  if (!regex_state.match_context) [[unlikely]] {
    return std::nullopt;
  }

  auto& runtime_ctx{RuntimeContext::get()};
  PCRE2_SIZE buffer_length{std::max({subject.size(), static_cast<string::size_type>(RegexInstanceState::REPLACE_BUFFER_SIZE), runtime_ctx.static_SB.size()})};
  runtime_ctx.static_SB.clean().reserve(buffer_length);

  size_t last_pos{};
  string output_str{};

  kphp::log::assertion(replacement.has_value());

  kphp::pcre2::matcher pcre2_matcher{re, {subject.c_str(), subject.size()}, {}, regex_state.match_context, regex_state.match_data, regex.match_options};
  while (replace_count < limit) {
    auto expected_opt_match_view{pcre2_matcher.next()};

    if (!expected_opt_match_view.has_value()) [[unlikely]] {
      kphp::log::warning("can't replace by pcre2 regex due to match error: {}", expected_opt_match_view.error());
      return std::nullopt;
    }
    auto opt_match_view{*expected_opt_match_view};
    if (!opt_match_view.has_value()) {
      break;
    }

    auto& match_view{*opt_match_view};

    output_str.append(std::next(subject.c_str(), last_pos), match_view.match_start() - last_pos);

    auto sub_res{
        match_view.substitute({replacement->c_str(), replacement->size()}, {runtime_ctx.static_SB.buffer(), buffer_length}, regex_state.match_context)};
    if (!sub_res.has_value()) {
      auto [needed_size, error]{sub_res.error()};
      if (error.code == PCRE2_ERROR_NOMEMORY) [[unlikely]] {
        runtime_ctx.static_SB.reserve(needed_size);
        buffer_length = needed_size;
        sub_res =
            match_view.substitute({replacement->c_str(), replacement->size()}, {runtime_ctx.static_SB.buffer(), buffer_length}, regex_state.match_context);
      }
      if (!sub_res.has_value()) [[unlikely]] {
        kphp::log::warning("pcre2_substitute error {}", sub_res.error().second);
        return std::nullopt;
      }
    }

    output_str.append(runtime_ctx.static_SB.buffer(), *sub_res);

    last_pos = match_view.match_end();
    ++replace_count;
  }

  output_str.append(std::next(subject.c_str(), last_pos), subject.size() - last_pos);

  return output_str;
}

inline std::optional<array<mixed>> split_regex(const kphp::pcre2::regex& re, const string& subject, int64_t limit, const uint32_t match_options,
                                               const bool no_empty, const bool delim_capture, const bool offset_capture) noexcept {
  if (limit == 0) {
    limit = kphp::regex::PREG_NOLIMIT;
  }

  auto& regex_state{RegexInstanceState::get()};
  if (!regex_state.match_context) [[unlikely]] {
    return std::nullopt;
  }

  array<mixed> output{};

  kphp::pcre2::matcher pcre2_matcher{re, {subject.c_str(), subject.size()}, {}, regex_state.match_context, regex_state.match_data, match_options};
  size_t offset{};
  for (size_t out_parts_count{1}; limit == kphp::regex::PREG_NOLIMIT || out_parts_count < limit;) {
    auto expected_opt_match_view{pcre2_matcher.next()};
    if (!expected_opt_match_view.has_value()) [[unlikely]] {
      kphp::log::warning("can't split by pcre2 regex due to match error: {}", expected_opt_match_view.error());
      return std::nullopt;
    }
    auto opt_match_view{*expected_opt_match_view};
    if (!opt_match_view.has_value()) {
      break;
    }

    kphp::pcre2::match_view match_view{*opt_match_view};

    if (const auto size{match_view.match_start() - offset}; !no_empty || size != 0) {
      string val{std::next(subject.c_str(), offset), static_cast<string::size_type>(size)};

      mixed output_val;
      if (offset_capture) {
        output_val = array<mixed>::create(std::move(val), static_cast<int64_t>(offset));
      } else {
        output_val = std::move(val);
      }

      output.emplace_back(std::move(output_val));
      ++out_parts_count;
    }

    if (delim_capture) {
      for (size_t i{1}; i < match_view.size(); i++) {
        auto opt_submatch{match_view.get_group(i)};
        auto submatch_string_view{opt_submatch.value_or(std::string_view{})};
        const auto size{submatch_string_view.size()};
        if (!no_empty || size != 0) {
          string val;
          if (opt_submatch.has_value()) [[likely]] {
            val = string{submatch_string_view.data(), static_cast<string::size_type>(size)};
          }

          mixed output_val;
          if (offset_capture) {
            output_val = array<mixed>::create(std::move(val), opt_submatch
                                                                  .transform([&subject](auto submatch_string_view) noexcept {
                                                                    return static_cast<int64_t>(std::distance(subject.c_str(), submatch_string_view.data()));
                                                                  })
                                                                  .value_or(-1));
          } else {
            output_val = std::move(val);
          }

          output.emplace_back(std::move(output_val));
        }
      }
    }

    offset = match_view.match_end();
  }

  const auto size{subject.size() - offset};
  if (!no_empty || size != 0) {
    string val{std::next(subject.c_str(), offset), static_cast<string::size_type>(size)};

    mixed output_val;
    if (offset_capture) {
      output_val = array<mixed>::create(std::move(val), static_cast<int64_t>(offset));
    } else {
      output_val = std::move(val);
    }

    output.emplace_back(std::move(output_val));
  }

  return output;
}

class preg_replacement_parser {
  std::string_view preg_replacement;

  replacement_term parse_term_internal() noexcept {
    kphp::log::assertion(!preg_replacement.empty());
    auto first_char{preg_replacement.front()};
    preg_replacement = preg_replacement.substr(1);
    if (preg_replacement.empty()) {
      return first_char;
    }
    switch (first_char) {
    case '$':
      // $1, ${1}
      if (preg_replacement.front() == '{') {
        return try_get_backref(preg_replacement.substr(1))
            .and_then([this](auto br) noexcept -> std::optional<replacement_term> {
              auto digits_end_pos{1 + br.size()};
              if (digits_end_pos < preg_replacement.size() && preg_replacement[digits_end_pos] == '}') {
                preg_replacement = preg_replacement.substr(1 + br.size() + 1);
                return br;
              }

              return std::nullopt;
            })
            .value_or('$');
      }

      return try_get_backref(preg_replacement)
          .transform([this](auto br) noexcept -> replacement_term {
            auto digits_end_pos{br.size()};
            preg_replacement = preg_replacement.substr(digits_end_pos);
            return br;
          })
          .value_or('$');

    case '\\': {
      // \1
      auto opt_back_reference{try_get_backref(preg_replacement).transform([this](auto br) noexcept -> replacement_term {
        auto digits_end_pos{br.size()};
        preg_replacement = preg_replacement.substr(digits_end_pos);
        return br;
      })};
      if (opt_back_reference.has_value()) {
        return *opt_back_reference;
      } else {
        auto c{preg_replacement.front()};
        if (c == '$' || c == '\\') {
          preg_replacement = preg_replacement.substr(1);
          return c;
        }
        return '\\';
      }
    }
    default:
      return first_char;
    }
  }

public:
  explicit preg_replacement_parser(std::string_view preg_replacement) noexcept
      : preg_replacement{preg_replacement} {}

  struct iterator {
    preg_replacement_parser* parser{nullptr};
    replacement_term current_term{'\0'};

    using difference_type = std::ptrdiff_t;
    using value_type = replacement_term;
    using reference = const replacement_term&;
    using pointer = const replacement_term*;
    using iterator_category = std::input_iterator_tag;

    iterator() noexcept = default;
    explicit iterator(preg_replacement_parser* p) noexcept
        : parser{p} {
      if (parser->preg_replacement.empty()) {
        parser = nullptr;
      } else {
        current_term = parser->parse_term_internal();
      }
    }

    reference operator*() const noexcept {
      return current_term;
    }
    pointer operator->() const noexcept {
      return std::addressof(current_term);
    }

    iterator& operator++() noexcept {
      if (!parser->preg_replacement.empty()) {
        current_term = parser->parse_term_internal();
      } else {
        parser = nullptr;
      }
      return *this;
    }
    iterator operator++(int) noexcept { // NOLINT
      iterator temp{*this};
      ++(*this);
      return temp;
    }

    friend bool operator==(const iterator& a, const iterator& b) noexcept {
      return a.parser == b.parser;
    }
    friend bool operator!=(const iterator& a, const iterator& b) noexcept {
      return !(a == b);
    }
  };

  iterator begin() noexcept {
    return iterator{this};
  }
  iterator end() noexcept {
    return iterator{};
  }
};

class match_results_wrapper {
  const pcre2::match_view& m_view;
  const kphp::stl::vector<kphp::pcre2::group_name, kphp::memory::script_allocator>& m_group_names;
  uint32_t m_capture_count;
  uint32_t m_name_count;
  trailing_unmatch m_last_unmatched_policy;
  bool m_is_offset_capture;
  bool m_is_unmatched_as_null;

public:
  match_results_wrapper(const pcre2::match_view& match_view, const kphp::stl::vector<kphp::pcre2::group_name, kphp::memory::script_allocator>& names,
                        const uint32_t capture_count, const uint32_t name_count, const trailing_unmatch last_unmatched_policy, const bool is_offset_capture,
                        const bool is_unmatched_as_null) noexcept
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
    const match_results_wrapper& m_parent;
    uint32_t m_group_idx;
    bool m_yield_name{false};

  public:
    using iterator_category = std::forward_iterator_tag;
    using value_type = std::pair<mixed, mixed>;
    using difference_type = std::ptrdiff_t;
    using pointer = value_type*;
    using reference = value_type;

    iterator(const match_results_wrapper& parent, const uint32_t group_idx) noexcept
        : m_parent{parent},
          m_group_idx{group_idx} {
      if (m_group_idx < m_parent.m_group_names.size() && !m_parent.m_group_names[m_group_idx].name.empty()) {
        m_yield_name = true;
      }
    }

    reference operator*() const noexcept;

    iterator& operator++() noexcept {
      if (m_yield_name) {
        m_yield_name = false;
      } else {
        m_group_idx++;

        if (m_group_idx < m_parent.m_group_names.size() && !m_parent.m_group_names[m_group_idx].name.empty()) {
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
  };

  iterator begin() const noexcept {
    return iterator{*this, 0};
  }

  iterator end() const noexcept {
    return iterator{*this, match_count()};
  }
};

inline array<mixed> to_mixed_array(const kphp::regex::details::match_results_wrapper& wrapper) noexcept {
  const bool numeric_only{wrapper.name_count() == 0};

  array<mixed> result_map{array_size{static_cast<int64_t>(wrapper.max_potential_size()), numeric_only}};
  for (auto [key, value] : wrapper) {
    result_map.set_value(key, value);
  }
  return result_map;
}

inline match_results_wrapper::iterator::reference match_results_wrapper::iterator::operator*() const noexcept {
  auto content_opt{m_parent.m_view.get_group_content(m_group_idx)};

  mixed val_mixed;

  mixed unmatched_val{m_parent.m_is_unmatched_as_null ? mixed{} : mixed{string{}}};

  if (m_parent.m_is_offset_capture) {
    val_mixed = content_opt ? array<mixed>::create(string{content_opt->text.data(), static_cast<string::size_type>(content_opt->text.size())},
                                                   static_cast<int64_t>(content_opt->offset))
                            : array<mixed>::create(unmatched_val, static_cast<int64_t>(-1));
  } else {
    val_mixed = content_opt ? string{content_opt->text.data(), static_cast<string::size_type>(content_opt->text.size())} : unmatched_val;
  }

  mixed key_mixed;
  if (m_yield_name) {
    auto name{m_parent.m_group_names[m_group_idx].name};
    key_mixed = string{name.data(), static_cast<string::size_type>(name.size())};
  } else {
    key_mixed = static_cast<int64_t>(m_group_idx);
  }

  return {key_mixed, val_mixed};
}

// *** importrant ***
// in case of a pattern order all_matches must already contain all groups as empty arrays before the first call to set_all_matches
inline void set_all_matches(const kphp::pcre2::regex& re, const kphp::stl::vector<kphp::pcre2::group_name, kphp::memory::script_allocator>& group_names,
                            const kphp::pcre2::match_view& match_view, const int64_t flags,
                            const std::optional<std::reference_wrapper<mixed>> opt_all_matches) noexcept {
  const auto is_pattern_order{!static_cast<bool>(flags & kphp::regex::PREG_SET_ORDER)};
  const auto is_offset_capture{static_cast<bool>(flags & kphp::regex::PREG_OFFSET_CAPTURE)};
  const auto is_unmatched_as_null{static_cast<bool>(flags & kphp::regex::PREG_UNMATCHED_AS_NULL)};

  // early return in case we don't actually need to set matches
  if (!opt_all_matches.has_value()) {
    return;
  }

  auto last_unmatched_policy{is_pattern_order ? kphp::regex::details::trailing_unmatch::include : kphp::regex::details::trailing_unmatch::skip};
  mixed matches{kphp::regex::details::to_mixed_array(
      {match_view, group_names, re.capture_count(), re.name_count(), last_unmatched_policy, is_offset_capture, is_unmatched_as_null})};

  mixed& all_matches{opt_all_matches->get()};
  if (is_pattern_order) [[likely]] {
    for (const auto& it : std::as_const(matches)) {
      all_matches[it.get_key()].push_back(it.get_value());
    }
  } else {
    all_matches.push_back(matches);
  }
}

/**
 * Collects all named capture groups and maps them to their group numbers.
 *
 * This function extracts the identifier for each named capture group and places
 * it into a vector where the index exactly matches the group's capture number.
 *
 * @param re The compiled PCRE2 regular expression to inspect.
 * @return A vector of group_name objects indexed by their group number.
 *         Index 0 (the whole match) and any unnamed group numbers will
 *         contain default/empty group_name values.
 * @noexcept
 */
inline kphp::stl::vector<kphp::pcre2::group_name, kphp::memory::script_allocator> collect_group_names(const pcre2::regex& re) noexcept {
  kphp::stl::vector<kphp::pcre2::group_name, kphp::memory::script_allocator> names;
  // initialize an array of strings to hold group names
  names.resize(re.capture_count() + 1);

  if (re.name_count() == 0) {
    return names;
  }

  for (const auto& entry : re.group_names()) {
    names[entry.index] = entry;
  }

  return names;
}

inline auto get_count_finalizer(int64_t& count, Optional<std::variant<std::monostate, std::reference_wrapper<int64_t>>>& opt_count) noexcept {
  return vk::final_action{[&count, &opt_count]() noexcept {
    if (opt_count.has_value()) {
      kphp::log::assertion(std::holds_alternative<std::reference_wrapper<int64_t>>(opt_count.val()));
      auto& inner_ref{std::get<std::reference_wrapper<int64_t>>(opt_count.val()).get()};
      inner_ref = count;
    }
  }};
}

inline bool preg_match_args_check(const string& subject, const int64_t flags, int64_t& offset) noexcept {
  if (!kphp::regex::details::valid_regex_flags(flags, kphp::regex::PREG_NO_FLAGS, kphp::regex::PREG_OFFSET_CAPTURE, kphp::regex::PREG_UNMATCHED_AS_NULL))
      [[unlikely]] {
    return false;
  }

  if (!kphp::regex::details::correct_offset(offset, {subject.c_str(), subject.size()})) [[unlikely]] {
    return false;
  }

  return true;
}

inline Optional<int64_t> preg_match_impl(const regexp& regex, const string& subject,
                                         Optional<std::variant<std::monostate, std::reference_wrapper<mixed>>> opt_matches, const int64_t flags,
                                         const int64_t offset) noexcept {
  const auto opt_re{regex.get_regex()};
  if (!opt_re.has_value()) [[unlikely]] {
    return false;
  }
  const kphp::pcre2::regex& re{opt_re->get()};
  auto group_names{kphp::regex::details::collect_group_names(re)};

  auto& regex_state{RegexInstanceState::get()};
  kphp::log::assertion(regex_state.match_context != nullptr);

  auto expected_opt_match_view{kphp::pcre2::matcher{
      re, {subject.c_str(), subject.size()}, static_cast<size_t>(offset), regex_state.match_context, regex_state.match_data, regex.match_options}
                                   .next()};
  if (!expected_opt_match_view.has_value()) [[unlikely]] {
    kphp::log::warning("can't match by pcre2 regex due to error: {}", expected_opt_match_view.error());
    return false;
  }
  auto opt_match_view{*expected_opt_match_view};

  if (opt_matches.has_value()) {
    const auto is_offset_capture{static_cast<bool>(flags & kphp::regex::PREG_OFFSET_CAPTURE)};
    const auto is_unmatched_as_null{static_cast<bool>(flags & kphp::regex::PREG_UNMATCHED_AS_NULL)};

    kphp::log::assertion(std::holds_alternative<std::reference_wrapper<mixed>>(opt_matches.val()));
    auto& inner_ref{std::get<std::reference_wrapper<mixed>>(opt_matches.val()).get()};
    inner_ref = array<mixed>{};
    opt_match_view.transform([is_offset_capture, is_unmatched_as_null, &inner_ref, &group_names, &re](const auto& match_view) {
      inner_ref = kphp::regex::details::to_mixed_array({match_view, group_names, re.capture_count(), re.name_count(),
                                                        kphp::regex::details::trailing_unmatch::skip, is_offset_capture, is_unmatched_as_null});
      return 0;
    });
  }
  return opt_match_view.has_value() ? 1 : 0;
}

inline bool preg_match_all_args_check(const string& subject, const int64_t flags, int64_t& offset) noexcept {
  if (!kphp::regex::details::valid_regex_flags(flags, kphp::regex::PREG_NO_FLAGS, kphp::regex::PREG_PATTERN_ORDER, kphp::regex::PREG_SET_ORDER,
                                               kphp::regex::PREG_OFFSET_CAPTURE, kphp::regex::PREG_UNMATCHED_AS_NULL)) [[unlikely]] {
    return false;
  }
  if (!kphp::regex::details::correct_offset(offset, {subject.c_str(), subject.size()})) [[unlikely]] {
    return false;
  }

  return true;
}

inline Optional<int64_t> preg_match_all_impl(const regexp& regex, const string& subject,
                                             Optional<std::variant<std::monostate, std::reference_wrapper<mixed>>> opt_matches, int64_t flags,
                                             int64_t offset) noexcept {
  auto opt_re{regex.get_regex()};
  if (!opt_re.has_value()) [[unlikely]] {
    return false;
  }

  int64_t entire_match_count{};
  const auto& re{*opt_re};
  auto group_names{kphp::regex::details::collect_group_names(re)};

  std::optional<std::reference_wrapper<mixed>> matches{};
  if (opt_matches.has_value()) {
    kphp::log::assertion(std::holds_alternative<std::reference_wrapper<mixed>>(opt_matches.val()));
    auto& inner_ref{std::get<std::reference_wrapper<mixed>>(opt_matches.val()).get()};
    inner_ref = array<mixed>{};
    matches.emplace(inner_ref);
  }

  // pre-init matches in case of pattern order
  if (matches.has_value() && !static_cast<bool>(flags & kphp::regex::PREG_SET_ORDER)) [[likely]] {
    auto& inner_ref{(*matches).get()};
    const array<mixed> init_val{};
    for (const auto [name, index] : group_names) {
      if (!name.empty()) {
        inner_ref.set_value(string{name.data(), static_cast<string::size_type>(name.size())}, init_val);
      }
      inner_ref.push_back(init_val);
    }
  }

  auto& regex_state{RegexInstanceState::get()};
  kphp::log::assertion(regex_state.match_context != nullptr);

  kphp::pcre2::matcher pcre2_matcher{
      re, {subject.c_str(), subject.size()}, static_cast<size_t>(offset), regex_state.match_context, regex_state.match_data, regex.match_options};

  while (true) {
    auto expected_opt_match_view{pcre2_matcher.next()};

    if (!expected_opt_match_view.has_value()) [[unlikely]] {
      kphp::log::warning("can't find all matches due to match error: {}", expected_opt_match_view.error());
      return false;
    }
    auto opt_match_view{*expected_opt_match_view};
    if (!opt_match_view.has_value()) {
      break;
    }

    kphp::pcre2::match_view match_view{*opt_match_view};
    kphp::regex::details::set_all_matches(re, group_names, match_view, flags, matches);
    ++entire_match_count;
  }

  return entire_match_count;
}

inline std::optional<string> preg_replace_preparing(const string& replacement, int64_t limit) noexcept {
  if (limit < 0 && limit != kphp::regex::PREG_NOLIMIT) [[unlikely]] {
    kphp::log::warning("invalid limit {} in preg_replace", limit);
    return std::nullopt;
  }

  // we need to replace PHP's back references with PCRE2 ones
  auto parser{kphp::regex::details::preg_replacement_parser{{replacement.c_str(), replacement.size()}}};
  string pcre2_replacement{};
  for (const auto& term : parser) {
    if (std::holds_alternative<char>(term)) {
      auto c{std::get<char>(term)};
      pcre2_replacement.push_back(c);
      if (c == '$') {
        pcre2_replacement.push_back('$');
      }
    } else {
      auto backreference{std::get<kphp::regex::details::backref>(term)};
      pcre2_replacement.reserve_at_least(pcre2_replacement.size() + backreference.size() + 3);
      pcre2_replacement.append("${");
      pcre2_replacement.append(backreference.data(), backreference.size());
      pcre2_replacement.append("}");
    }
  }
  return pcre2_replacement;
}

inline Optional<string> preg_replace_impl(const regexp& regex, const string& subject, const string& replacement, int64_t limit, int64_t& count) noexcept {

  auto opt_re{regex.get_regex()};
  if (!opt_re.has_value()) [[unlikely]] {
    return {};
  }

  const auto& re{opt_re->get()};
  int64_t replace_count{};
  auto opt_replace_result{kphp::regex::details::replace_regex(
      regex, subject, std::optional{replacement}, re, limit == kphp::regex::PREG_NOLIMIT ? std::numeric_limits<uint64_t>::max() : static_cast<uint64_t>(limit),
      replace_count)};
  if (!opt_replace_result.has_value()) {
    return {};
  }
  count = replace_count;
  return std::move(*opt_replace_result);
}

inline bool preg_replace_callback_check(int64_t limit = kphp::regex::PREG_NOLIMIT, int64_t flags = kphp::regex::PREG_NO_FLAGS) noexcept {
  if (limit < 0 && limit != kphp::regex::PREG_NOLIMIT) [[unlikely]] {
    kphp::log::warning("invalid limit {} in preg_replace_callback", limit);
    return false;
  }
  if (!kphp::regex::details::valid_regex_flags(flags, kphp::regex::PREG_NO_FLAGS, kphp::regex::PREG_OFFSET_CAPTURE, kphp::regex::PREG_UNMATCHED_AS_NULL))
      [[unlikely]] {
    return false;
  }
  return true;
}

template<std::invocable<array<string>> F>
kphp::coro::task<Optional<string>> preg_replace_callback_impl(const regexp& regex, F callback, string subject, int64_t& count,
                                                              const int64_t limit = kphp::regex::PREG_NOLIMIT) noexcept {
  static_assert(std::same_as<kphp::coro::async_function_return_type_t<F, array<string>>, string>);

  const auto opt_re{regex.get_regex()};
  if (!opt_re.has_value()) [[unlikely]] {
    co_return Optional<string>{};
  }
  const auto& re{opt_re->get()};
  auto group_names{kphp::regex::details::collect_group_names(re)};
  auto unsigned_limit{limit == kphp::regex::PREG_NOLIMIT ? std::numeric_limits<uint64_t>::max() : static_cast<uint64_t>(limit)};
  int64_t replace_count{};

  if (limit == 0) {
    count = replace_count;
    co_return subject;
  }

  auto& regex_state{RegexInstanceState::get()};
  if (!regex_state.match_context) [[unlikely]] {
    co_return Optional<string>{};
  }

  size_t last_pos{};
  string output_str{};

  kphp::pcre2::matcher pcre2_matcher{re, {subject.c_str(), subject.size()}, {}, regex_state.match_context, regex_state.match_data, regex.match_options};
  while (replace_count < unsigned_limit) {
    auto expected_opt_match_view{pcre2_matcher.next()};

    if (!expected_opt_match_view.has_value()) [[unlikely]] {
      kphp::log::warning("can't replace with callback by pcre2 regex due to match error: {}", expected_opt_match_view.error());
      co_return Optional<string>{};
    }
    auto opt_match_view{*expected_opt_match_view};
    if (!opt_match_view.has_value()) {
      break;
    }

    auto& match_view{*opt_match_view};

    output_str.append(std::next(subject.c_str(), last_pos), match_view.match_start() - last_pos);

    last_pos = match_view.match_end();

    // retrieve the named groups count
    uint32_t named_groups_count{re.name_count()};

    array<string> matches{array_size{static_cast<int64_t>(match_view.size() + named_groups_count), named_groups_count == 0}};
    for (auto [key, value] : kphp::regex::details::match_results_wrapper{match_view, group_names, re.capture_count(), re.name_count(),
                                                                         kphp::regex::details::trailing_unmatch::skip, false, false}) {
      matches.set_value(key, value.to_string());
    }
    string replacement{};
    if constexpr (kphp::coro::is_async_function_v<F, array<string>>) {
      replacement = co_await std::invoke(callback, std::move(matches));
    } else {
      replacement = std::invoke(callback, std::move(matches));
    }

    output_str.append(replacement);

    ++replace_count;
  }

  output_str.append(std::next(subject.c_str(), last_pos), subject.size() - last_pos);

  count = replace_count;
  co_return output_str;
}

inline bool preg_split_check(const int64_t flags) noexcept {
  return kphp::regex::details::valid_regex_flags(flags, kphp::regex::PREG_NO_FLAGS, kphp::regex::PREG_SPLIT_NO_EMPTY, kphp::regex::PREG_SPLIT_DELIM_CAPTURE,
                                                 kphp::regex::PREG_SPLIT_OFFSET_CAPTURE);
}

inline Optional<array<mixed>> preg_split_impl(const regexp& regex, const string& subject, const int64_t limit, const int64_t flags) noexcept {

  auto opt_re{regex.get_regex()};
  if (!opt_re.has_value()) [[unlikely]] {
    return {};
  }

  const auto& re{opt_re->get()};
  auto opt_output{kphp::regex::details::split_regex(re, subject, limit, regex.match_options, (flags & kphp::regex::PREG_SPLIT_NO_EMPTY) != 0,
                                                    (flags & kphp::regex::PREG_SPLIT_DELIM_CAPTURE) != 0,
                                                    (flags & kphp::regex::PREG_SPLIT_OFFSET_CAPTURE) != 0)};
  if (!opt_output.has_value()) [[unlikely]] {
    return false;
  }

  return std::move(*opt_output);
}

} // namespace details

} // namespace kphp::regex

// === preg_match =================================================================================

Optional<int64_t> f$preg_match(const kphp::regex::regexp& regex, const string& subject,
                               const Optional<std::variant<std::monostate, std::reference_wrapper<mixed>>>& opt_matches = {},
                               int64_t flags = kphp::regex::PREG_NO_FLAGS, int64_t offset = 0) noexcept;

Optional<int64_t> f$preg_match(const string& pattern, const string& subject,
                               const Optional<std::variant<std::monostate, std::reference_wrapper<mixed>>>& opt_matches = {},
                               int64_t flags = kphp::regex::PREG_NO_FLAGS, int64_t offset = 0) noexcept;

Optional<int64_t> f$preg_match(const mixed& pattern, const string& subject,
                               const Optional<std::variant<std::monostate, std::reference_wrapper<mixed>>>& opt_matches = {},
                               int64_t flags = kphp::regex::PREG_NO_FLAGS, int64_t offset = 0) noexcept;

// === preg_match_all =============================================================================

Optional<int64_t> f$preg_match_all(const kphp::regex::regexp& regex, const string& subject,
                                   const Optional<std::variant<std::monostate, std::reference_wrapper<mixed>>>& opt_matches = {},
                                   int64_t flags = kphp::regex::PREG_NO_FLAGS, int64_t offset = 0) noexcept;

Optional<int64_t> f$preg_match_all(const string& pattern, const string& subject,
                                   const Optional<std::variant<std::monostate, std::reference_wrapper<mixed>>>& opt_matches = {},
                                   int64_t flags = kphp::regex::PREG_NO_FLAGS, int64_t offset = 0) noexcept;

Optional<int64_t> f$preg_match_all(const mixed& pattern, const string& subject,
                                   const Optional<std::variant<std::monostate, std::reference_wrapper<mixed>>>& opt_matches = {},
                                   int64_t flags = kphp::regex::PREG_NO_FLAGS, int64_t offset = 0) noexcept;

// === preg_replace ===============================================================================

Optional<string> f$preg_replace(const kphp::regex::regexp& regex, const string& replacement, const string& subject, int64_t limit = kphp::regex::PREG_NOLIMIT,
                                Optional<std::variant<std::monostate, std::reference_wrapper<int64_t>>> opt_count = {}) noexcept;
Optional<string> f$preg_replace(const kphp::regex::regexp& regex, const mixed& replacement, const string& subject, int64_t limit = kphp::regex::PREG_NOLIMIT,
                                Optional<std::variant<std::monostate, std::reference_wrapper<int64_t>>> opt_count = {}) noexcept;
mixed f$preg_replace(const kphp::regex::regexp& regex, const string& replacement, const mixed& subject, int64_t limit = kphp::regex::PREG_NOLIMIT,
                     const Optional<std::variant<std::monostate, std::reference_wrapper<int64_t>>>& opt_count = {}) noexcept;
mixed f$preg_replace(const kphp::regex::regexp& regex, const mixed& replacement, const mixed& subject, int64_t limit = kphp::regex::PREG_NOLIMIT,
                     Optional<std::variant<std::monostate, std::reference_wrapper<int64_t>>> opt_count = {}) noexcept;

Optional<string> f$preg_replace(const string& pattern, const string& replacement, const string& subject, int64_t limit = kphp::regex::PREG_NOLIMIT,
                                Optional<std::variant<std::monostate, std::reference_wrapper<int64_t>>> opt_count = {}) noexcept;

Optional<string> f$preg_replace(const mixed& pattern, const string& replacement, const string& subject, int64_t limit = kphp::regex::PREG_NOLIMIT,
                                Optional<std::variant<std::monostate, std::reference_wrapper<int64_t>>> opt_count = {}) noexcept;
mixed f$preg_replace(const mixed& pattern, const string& replace_val, const mixed& subject, int64_t limit = kphp::regex::PREG_NOLIMIT,
                     const Optional<std::variant<std::monostate, std::reference_wrapper<int64_t>>>& opt_count = {}) noexcept;
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
kphp::coro::task<Optional<string>> f$preg_replace_callback(const kphp::regex::regexp& regex, F callback, string subject,
                                                           int64_t limit = kphp::regex::PREG_NOLIMIT,
                                                           Optional<std::variant<std::monostate, std::reference_wrapper<int64_t>>> opt_count = {},
                                                           int64_t flags = kphp::regex::PREG_NO_FLAGS) noexcept;

template<class F>
kphp::coro::task<mixed> f$preg_replace_callback(const kphp::regex::regexp& regex, F callback, const mixed& subject, int64_t limit = kphp::regex::PREG_NOLIMIT,
                                                Optional<std::variant<std::monostate, std::reference_wrapper<int64_t>>> opt_count = {},
                                                int64_t flags = kphp::regex::PREG_NO_FLAGS) noexcept;

template<std::invocable<array<string>> F>
kphp::coro::task<Optional<string>> f$preg_replace_callback(string pattern, F callback, string subject, int64_t limit = kphp::regex::PREG_NOLIMIT,
                                                           Optional<std::variant<std::monostate, std::reference_wrapper<int64_t>>> opt_count = {},
                                                           int64_t flags = kphp::regex::PREG_NO_FLAGS) noexcept;

template<class F>
kphp::coro::task<Optional<string>> f$preg_replace_callback(mixed pattern, F callback, string subject, int64_t limit = kphp::regex::PREG_NOLIMIT,
                                                           Optional<std::variant<std::monostate, std::reference_wrapper<int64_t>>> opt_count = {},
                                                           int64_t flags = kphp::regex::PREG_NO_FLAGS) noexcept;

template<class F>
kphp::coro::task<mixed> f$preg_replace_callback(mixed pattern, F callback, mixed subject, int64_t limit = kphp::regex::PREG_NOLIMIT,
                                                Optional<std::variant<std::monostate, std::reference_wrapper<int64_t>>> opt_count = {},
                                                int64_t flags = kphp::regex::PREG_NO_FLAGS) noexcept;

template<class T1, class T2, class T3, class = enable_if_t_is_optional<T3>>
auto f$preg_replace_callback(T1&& pattern, T2&& callback, T3&& subject, int64_t limit = kphp::regex::PREG_NOLIMIT,
                             Optional<std::variant<std::monostate, std::reference_wrapper<int64_t>>> opt_count = {},
                             int64_t flags = kphp::regex::PREG_NO_FLAGS) noexcept
    -> decltype(f$preg_replace_callback(std::forward<T1>(pattern), std::forward<T2>(callback), std::forward<T3>(subject).val(), limit, opt_count, flags));

// === preg_split =================================================================================

Optional<array<mixed>> f$preg_split(const kphp::regex::regexp& regex, const string& subject, int64_t limit = kphp::regex::PREG_NOLIMIT,
                                    int64_t flags = kphp::regex::PREG_NO_FLAGS) noexcept;

Optional<array<mixed>> f$preg_split(const string& pattern, const string& subject, int64_t limit = kphp::regex::PREG_NOLIMIT,
                                    int64_t flags = kphp::regex::PREG_NO_FLAGS) noexcept;

Optional<array<mixed>> f$preg_split(const mixed& pattern, const string& subject, int64_t limit = kphp::regex::PREG_NOLIMIT,
                                    int64_t flags = kphp::regex::PREG_NO_FLAGS) noexcept;

// ==================================================================================================================================================

// === preg_match implementation ==================================================================

inline Optional<int64_t> f$preg_match(const kphp::regex::regexp& regex, const string& subject,
                                      const Optional<std::variant<std::monostate, std::reference_wrapper<mixed>>>& opt_matches, const int64_t flags,
                                      int64_t offset) noexcept {
  if (!kphp::regex::details::preg_match_args_check(subject, flags, offset)) [[unlikely]] {
    return false;
  }
  return kphp::regex::details::preg_match_impl(regex, subject, opt_matches, flags, offset);
}

inline Optional<int64_t> f$preg_match(const string& pattern, const string& subject,
                                      const Optional<std::variant<std::monostate, std::reference_wrapper<mixed>>>& opt_matches, const int64_t flags,
                                      int64_t offset) noexcept {
  if (!kphp::regex::details::preg_match_args_check(subject, flags, offset)) [[unlikely]] {
    return false;
  }
  return kphp::regex::details::preg_match_impl(kphp::regex::regexp{pattern, subject}, subject, opt_matches, flags, offset);
}

inline Optional<int64_t> f$preg_match(const mixed& pattern, const string& subject,
                                      const Optional<std::variant<std::monostate, std::reference_wrapper<mixed>>>& opt_matches, const int64_t flags,
                                      int64_t offset) noexcept {
  return f$preg_match(pattern.to_string(), subject, opt_matches, flags, offset);
}

// === preg_match_all implementation ==============================================================

inline Optional<int64_t> f$preg_match_all(const kphp::regex::regexp& regex, const string& subject,
                                          const Optional<std::variant<std::monostate, std::reference_wrapper<mixed>>>& opt_matches, const int64_t flags,
                                          int64_t offset) noexcept {
  if (!kphp::regex::details::preg_match_all_args_check(subject, flags, offset)) [[unlikely]] {
    return false;
  }
  return kphp::regex::details::preg_match_all_impl(regex, subject, opt_matches, flags, offset);
}

inline Optional<int64_t> f$preg_match_all(const string& pattern, const string& subject,
                                          const Optional<std::variant<std::monostate, std::reference_wrapper<mixed>>>& opt_matches, const int64_t flags,
                                          int64_t offset) noexcept {
  if (!kphp::regex::details::preg_match_all_args_check(subject, flags, offset)) [[unlikely]] {
    return false;
  }
  return kphp::regex::details::preg_match_all_impl(kphp::regex::regexp{pattern, subject}, subject, opt_matches, flags, offset);
}

inline Optional<int64_t> f$preg_match_all(const mixed& pattern, const string& subject,
                                          const Optional<std::variant<std::monostate, std::reference_wrapper<mixed>>>& opt_matches, const int64_t flags,
                                          const int64_t offset) noexcept {
  return f$preg_match_all(pattern.to_string(), subject, opt_matches, flags, offset);
}

// === preg_replace part of implementation ========================================================

inline Optional<string> f$preg_replace(const kphp::regex::regexp& regex, const string& replacement, const string& subject, const int64_t limit,
                                       Optional<std::variant<std::monostate, std::reference_wrapper<int64_t>>> opt_count) noexcept {
  int64_t count{};
  auto count_finalizer{kphp::regex::details::get_count_finalizer(count, opt_count)};
  const auto& pcre2_replacement{kphp::regex::details::preg_replace_preparing(replacement, limit)};
  if (!pcre2_replacement.has_value()) [[unlikely]] {
    return false;
  }
  return kphp::regex::details::preg_replace_impl(regex, subject, pcre2_replacement.value(), limit, count);
}

inline Optional<string> f$preg_replace(const kphp::regex::regexp& regex, const mixed& replacement, const string& subject, int64_t limit,
                                       Optional<std::variant<std::monostate, std::reference_wrapper<int64_t>>> opt_count) noexcept {
  int64_t count{};
  auto count_finalizer{kphp::regex::details::get_count_finalizer(count, opt_count)};

  if (replacement.is_array()) {
    kphp::log::warning("parameter mismatch, pattern is a string while replacement is an array");
    return false;
  }

  return f$preg_replace(regex, replacement.to_string(), subject, limit, opt_count);
}

inline mixed f$preg_replace(const kphp::regex::regexp& regex, const string& replacement, const mixed& subject, int64_t limit,
                            const Optional<std::variant<std::monostate, std::reference_wrapper<int64_t>>>& opt_count) noexcept {
  return f$preg_replace(regex, mixed{replacement}, subject, limit, opt_count);
}

inline Optional<string> f$preg_replace(const string& pattern, const string& replacement, const string& subject, const int64_t limit,
                                       Optional<std::variant<std::monostate, std::reference_wrapper<int64_t>>> opt_count) noexcept {
  int64_t count{};
  auto count_finalizer{kphp::regex::details::get_count_finalizer(count, opt_count)};
  const auto& pcre2_replacement{kphp::regex::details::preg_replace_preparing(replacement, limit)};
  if (!pcre2_replacement.has_value()) [[unlikely]] {
    return false;
  }
  const kphp::regex::regexp regex{pattern, subject};
  return kphp::regex::details::preg_replace_impl(regex, subject, pcre2_replacement.value(), limit, count);
}

inline mixed f$preg_replace(const mixed& pattern, const string& replace_val, const mixed& subject, const int64_t limit,
                            const Optional<std::variant<std::monostate, std::reference_wrapper<int64_t>>>& opt_count) noexcept {
  return f$preg_replace(pattern, mixed{replace_val}, subject, limit, opt_count);
}

// === preg_replace_callback implementation =======================================================

template<std::invocable<array<string>> F>
inline kphp::coro::task<Optional<string>> f$preg_replace_callback(const kphp::regex::regexp& regex, F callback, string subject, int64_t limit,
                                                                  Optional<std::variant<std::monostate, std::reference_wrapper<int64_t>>> opt_count,
                                                                  int64_t flags) noexcept {
  static_assert(std::same_as<kphp::coro::async_function_return_type_t<F, array<string>>, string>);
  int64_t count{};
  auto count_finalizer{kphp::regex::details::get_count_finalizer(count, opt_count)};
  if (!kphp::regex::details::preg_replace_callback_check(limit, flags)) [[unlikely]] {
    co_return Optional<string>{};
  }

  co_return co_await kphp::regex::details::preg_replace_callback_impl(regex, callback, subject, count, limit);
}

template<class F>
kphp::coro::task<mixed> f$preg_replace_callback(const kphp::regex::regexp& regex, F callback, const mixed& subject, int64_t limit,
                                                Optional<std::variant<std::monostate, std::reference_wrapper<int64_t>>> opt_count, int64_t flags) noexcept {
  if (subject.is_object()) [[unlikely]] {
    kphp::log::warning("invalid subject: object could not be converted to string");
    co_return mixed{};
  }

  if (!subject.is_array()) {
    co_return co_await f$preg_replace_callback(regex, std::move(callback), subject.to_string(), limit, opt_count, flags);
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
    if (auto replace_result{co_await f$preg_replace_callback(regex, callback, it.get_value().to_string(), limit, replace_one_count, flags)};
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

template<std::invocable<array<string>> F>
kphp::coro::task<Optional<string>> f$preg_replace_callback(string pattern, F callback, string subject, int64_t limit,
                                                           Optional<std::variant<std::monostate, std::reference_wrapper<int64_t>>> opt_count,
                                                           int64_t flags) noexcept {
  static_assert(std::same_as<kphp::coro::async_function_return_type_t<F, array<string>>, string>);
  int64_t count{};
  auto count_finalizer{kphp::regex::details::get_count_finalizer(count, opt_count)};
  if (!kphp::regex::details::preg_replace_callback_check(limit, flags)) [[unlikely]] {
    co_return Optional<string>{};
  }
  const kphp::regex::regexp regex{pattern, subject};
  co_return co_await kphp::regex::details::preg_replace_callback_impl(regex, callback, subject, count, limit);
}

template<class F>
kphp::coro::task<Optional<string>> f$preg_replace_callback(mixed pattern, F callback, string subject, int64_t limit,
                                                           Optional<std::variant<std::monostate, std::reference_wrapper<int64_t>>> opt_count,
                                                           int64_t flags) noexcept {
  if (pattern.is_object()) [[unlikely]] {
    kphp::log::warning("invalid pattern: object could not be converted to string");
    co_return Optional<string>{};
  }

  if (!pattern.is_array()) {
    co_return co_await f$preg_replace_callback(pattern.to_string(), std::move(callback), std::move(subject), limit, opt_count, flags);
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
kphp::coro::task<mixed> f$preg_replace_callback(mixed pattern, F callback, mixed subject, int64_t limit,
                                                Optional<std::variant<std::monostate, std::reference_wrapper<int64_t>>> opt_count, int64_t flags) noexcept {
  if (pattern.is_object()) [[unlikely]] {
    kphp::log::warning("invalid pattern: object could not be converted to string");
    co_return mixed{};
  }
  if (subject.is_object()) [[unlikely]] {
    kphp::log::warning("invalid subject: object could not be converted to string");
    co_return mixed{};
  }

  if (!subject.is_array()) {
    co_return co_await f$preg_replace_callback(std::move(pattern), std::move(callback), subject.to_string(), limit, opt_count, flags);
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

template<class T1, class T2, class T3, class>
auto f$preg_replace_callback(T1&& pattern, T2&& callback, T3&& subject, int64_t limit,
                             Optional<std::variant<std::monostate, std::reference_wrapper<int64_t>>> opt_count,
                             int64_t flags) noexcept -> decltype(f$preg_replace_callback(std::forward<T1>(pattern), std::forward<T2>(callback),
                                                                                         std::forward<T3>(subject).val(), limit, opt_count, flags)) {
  co_return co_await f$preg_replace_callback(std::forward<T1>(pattern), std::forward<T2>(callback), std::forward<T3>(subject).val(), limit, opt_count, flags);
}

// === preg_split implementation ==================================================================

inline Optional<array<mixed>> f$preg_split(const kphp::regex::regexp& regex, const string& subject, const int64_t limit, const int64_t flags) noexcept {
  if (!kphp::regex::details::preg_split_check(flags)) {
    return false;
  }
  return kphp::regex::details::preg_split_impl(regex, subject, limit, flags);
}

inline Optional<array<mixed>> f$preg_split(const string& pattern, const string& subject, const int64_t limit, const int64_t flags) noexcept {
  if (!kphp::regex::details::preg_split_check(flags)) {
    return false;
  }
  return kphp::regex::details::preg_split_impl(kphp::regex::regexp{pattern, subject}, subject, limit, flags);
}

inline Optional<array<mixed>> f$preg_split(const mixed& pattern, const string& subject, int64_t limit, int64_t flags) noexcept {
  if (!pattern.is_string()) [[unlikely]] {
    kphp::log::warning("preg_split() expects parameter 1 to be string, {} given", pattern.get_type_or_class_name());
    return false;
  }
  return f$preg_split(pattern.as_string(), subject, limit, flags);
}
