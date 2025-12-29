// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime-light/stdlib/string/regex-functions.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <cstddef>
#include <cstdint>
#include <expected>
#include <functional>
#include <iterator>
#include <limits>
#include <memory>
#include <optional>
#include <string_view>
#include <type_traits>
#include <utility>
#include <variant>

#include "common/containers/final_action.h"
#include "runtime-common/core/allocator/script-allocator.h"
#include "runtime-common/core/runtime-core.h"
#include "runtime-common/core/std/containers.h"
#include "runtime-common/stdlib/string/mbstring-functions.h"
#include "runtime-light/stdlib/diagnostics/logs.h"
#include "runtime-light/stdlib/string/regex-include.h"
#include "runtime-light/stdlib/string/regex-state.h"

namespace {

using backref = std::string_view;

bool correct_offset(int64_t& offset, std::string_view subject) noexcept {
  if (offset < 0) [[unlikely]] {
    offset += subject.size();
    if (offset < 0) [[unlikely]] {
      offset = 0;
      return true;
    }
  }
  return offset <= subject.size();
}

std::optional<backref> try_get_backref(std::string_view preg_replacement) noexcept {
  if (preg_replacement.empty() || !std::isdigit(preg_replacement[0])) {
    return std::nullopt;
  }

  if (preg_replacement.size() == 1 || !std::isdigit(preg_replacement[1])) {
    return backref{preg_replacement.substr(0, 1)};
  }

  return backref{preg_replacement.substr(0, 2)};
}

using replacement_term = std::variant<char, backref>;

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

array<mixed> to_mixed_array(const kphp::regex::details::match_results_wrapper& wrapper) noexcept {
  const bool numeric_only{wrapper.name_count() == 0};

  array<mixed> result_map{array_size{static_cast<int64_t>(wrapper.max_potential_size()), numeric_only}};
  for (auto [key, value] : wrapper) {
    result_map.set_value(key, value);
  }
  return result_map;
}

// *** importrant ***
// in case of a pattern order all_matches must already contain all groups as empty arrays before the first call to set_all_matches
void set_all_matches(const kphp::pcre2::regex& re, const kphp::regex::details::pcre2_group_names_t& group_names, const kphp::pcre2::match_view& match_view,
                     int64_t flags, std::optional<std::reference_wrapper<mixed>> opt_all_matches) noexcept {
  const auto is_pattern_order{!static_cast<bool>(flags & kphp::regex::PREG_SET_ORDER)};
  const auto is_offset_capture{static_cast<bool>(flags & kphp::regex::PREG_OFFSET_CAPTURE)};
  const auto is_unmatched_as_null{static_cast<bool>(flags & kphp::regex::PREG_UNMATCHED_AS_NULL)};

  // early return in case we don't actually need to set matches
  if (!opt_all_matches.has_value()) {
    return;
  }

  auto last_unmatched_policy{is_pattern_order ? kphp::regex::details::trailing_unmatch::include : kphp::regex::details::trailing_unmatch::skip};
  mixed matches{to_mixed_array({match_view, group_names, re.capture_count(), re.name_count(), last_unmatched_policy, is_offset_capture, is_unmatched_as_null})};

  mixed& all_matches{(*opt_all_matches).get()};
  if (is_pattern_order) [[likely]] {
    for (const auto& it : std::as_const(matches)) {
      all_matches[it.get_key()].push_back(it.get_value());
    }
  } else {
    all_matches.push_back(matches);
  }
}

std::optional<string> replace_regex(kphp::regex::details::Info& regex_info, const kphp::pcre2::regex& re, uint64_t limit) noexcept {
  regex_info.replace_count = 0;

  if (limit == 0) {
    return regex_info.subject;
  }

  const auto& regex_state{RegexInstanceState::get()};
  if (!regex_state.match_context) [[unlikely]] {
    return std::nullopt;
  }

  auto& runtime_ctx{RuntimeContext::get()};
  PCRE2_SIZE buffer_length{
      std::max({regex_info.subject.size(), static_cast<string::size_type>(RegexInstanceState::REPLACE_BUFFER_SIZE), runtime_ctx.static_SB.size()})};
  runtime_ctx.static_SB.clean().reserve(buffer_length);

  size_t last_pos{};
  string output_str{};

  kphp::pcre2::matcher pcre2_matcher{re,
                                     {regex_info.subject.c_str(), regex_info.subject.size()},
                                     {},
                                     regex_state.match_context.get(),
                                     *regex_state.regex_pcre2_match_data.get(),
                                     regex_info.match_options};
  while (regex_info.replace_count < limit) {
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

    output_str.append(std::next(regex_info.subject.c_str(), last_pos), match_view.match_start() - last_pos);

    PCRE2_SIZE replacement_length{buffer_length};
    auto sub_res{re.substitute_match({regex_info.subject.c_str(), regex_info.subject.size()}, *regex_state.regex_pcre2_match_data.get(), regex_info.replacement,
                                     runtime_ctx.static_SB.buffer(), replacement_length, pcre2_matcher.get_last_success_options(),
                                     regex_state.match_context.get())};
    if (!sub_res.has_value() && sub_res.error().code == PCRE2_ERROR_NOMEMORY) [[unlikely]] {
      runtime_ctx.static_SB.reserve(replacement_length);
      buffer_length = replacement_length;
      sub_res =
          re.substitute_match({regex_info.subject.c_str(), regex_info.subject.size()}, *regex_state.regex_pcre2_match_data.get(), regex_info.replacement,
                              runtime_ctx.static_SB.buffer(), replacement_length, pcre2_matcher.get_last_success_options(), regex_state.match_context.get());
    }
    if (!sub_res.has_value()) [[unlikely]] {
      kphp::log::warning("pcre2_substitute error {}", sub_res.error());
      return std::nullopt;
    }

    output_str.append(runtime_ctx.static_SB.buffer(), replacement_length);

    last_pos = match_view.match_end();
    ++regex_info.replace_count;
  }

  output_str.append(std::next(regex_info.subject.c_str(), last_pos), regex_info.subject.size() - last_pos);

  return output_str;
}

std::optional<array<mixed>> split_regex(kphp::regex::details::Info& regex_info, const kphp::pcre2::regex& re, int64_t limit, bool no_empty, bool delim_capture,
                                        bool offset_capture) noexcept {
  if (limit == 0) {
    limit = kphp::regex::PREG_NOLIMIT;
  }

  const auto& regex_state{RegexInstanceState::get()};
  if (!regex_state.match_context) [[unlikely]] {
    return std::nullopt;
  }

  array<mixed> output{};

  kphp::pcre2::matcher pcre2_matcher{re,
                                     {regex_info.subject.c_str(), regex_info.subject.size()},
                                     {},
                                     regex_state.match_context.get(),
                                     *regex_state.regex_pcre2_match_data.get(),
                                     regex_info.match_options};
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
      string val{std::next(regex_info.subject.c_str(), offset), static_cast<string::size_type>(size)};

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
            output_val =
                array<mixed>::create(std::move(val), opt_submatch
                                                         .transform([&regex_info](auto submatch_string_view) noexcept {
                                                           return static_cast<int64_t>(std::distance(regex_info.subject.c_str(), submatch_string_view.data()));
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

  const auto size{regex_info.subject.size() - offset};
  if (!no_empty || size != 0) {
    string val{std::next(regex_info.subject.c_str(), offset), static_cast<string::size_type>(size)};

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

} // namespace

namespace kphp::regex {

namespace details {

match_pair match_results_wrapper::iterator::operator*() const noexcept {
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
    key_mixed = string{m_parent.m_group_names[m_group_idx]};
  } else {
    key_mixed = static_cast<int64_t>(m_group_idx);
  }

  return {.key = key_mixed, .value = val_mixed};
}

std::optional<std::reference_wrapper<const pcre2::regex>> compile_regex(Info& regex_info) noexcept {
  auto& regex_state{RegexInstanceState::get()};
  if (!regex_state.compile_context) [[unlikely]] {
    return std::nullopt;
  }

  // check runtime cache
  if (auto opt_ref{regex_state.get_compiled_regex(regex_info.regex)}; opt_ref.has_value()) {
    const auto& [compile_options, regex_code]{opt_ref->get()};
    regex_info.compile_options = compile_options;
    return regex_code;
  }

  if (regex_info.regex.empty()) {
    kphp::log::warning("empty regex");
    return std::nullopt;
  }

  char end_delim{};
  switch (const char start_delim{regex_info.regex[0]}; start_delim) {
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
    return std::nullopt;
  }
  }

  uint32_t compile_options{};
  std::string_view regex_body{regex_info.regex.c_str(), regex_info.regex.size()};

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
    kphp::log::warning("no ending regex delimiter: {}", regex_info.regex.c_str());
    return std::nullopt;
  }
  // UTF-8 validation
  if (static_cast<bool>(compile_options & PCRE2_UTF)) {
    if (!mb_UTF8_check(regex_info.regex.c_str())) [[unlikely]] {
      kphp::log::warning("invalid UTF-8 pattern: {}", regex_info.regex.c_str());
      return std::nullopt;
    }
    if (!mb_UTF8_check(regex_info.subject.c_str())) [[unlikely]] {
      kphp::log::warning("invalid UTF-8 subject: {}", regex_info.subject.c_str());
      return std::nullopt;
    }
  }

  // remove the end delimiter
  regex_body.remove_suffix(1);
  regex_info.compile_options = compile_options;

  // compile pcre2_code
  auto expected_re{pcre2::regex::compile(regex_body, regex_info.compile_options, regex_state.compile_context.get())};
  if (!expected_re.has_value()) [[unlikely]] {
    const auto& err{expected_re.error()};
    kphp::log::warning("can't compile pcre2 regex due to error at offset {}: {}", err.offset, err);
    return std::nullopt;
  }

  auto& re{*expected_re};
  // add compiled code to runtime cache
  return regex_state.add_compiled_regex(regex_info.regex, compile_options, std::move(re))->get().regex_code;
}

pcre2_group_names_t collect_group_names(const pcre2::regex& re) noexcept {
  // vector of group names
  pcre2_group_names_t group_names;
  // initialize an array of strings to hold group names
  group_names.resize(re.capture_count() + 1);

  if (re.name_count() == 0) {
    return group_names;
  }

  for (auto [name, group_number] : re.names()) {
    group_names[group_number] = name.data();
  }

  return group_names;
}

} // namespace details

} // namespace kphp::regex

Optional<int64_t> f$preg_match(const string& pattern, const string& subject, Optional<std::variant<std::monostate, std::reference_wrapper<mixed>>> opt_matches,
                               int64_t flags, int64_t offset) noexcept {
  kphp::regex::details::Info regex_info{pattern, subject, {}};

  if (!kphp::regex::details::valid_regex_flags(flags, kphp::regex::PREG_NO_FLAGS, kphp::regex::PREG_OFFSET_CAPTURE, kphp::regex::PREG_UNMATCHED_AS_NULL))
      [[unlikely]] {
    return false;
  }
  if (!correct_offset(offset, {regex_info.subject.c_str(), regex_info.subject.size()})) [[unlikely]] {
    return false;
  }
  auto opt_re{kphp::regex::details::compile_regex(regex_info)};
  if (!opt_re.has_value()) [[unlikely]] {
    return false;
  }
  const kphp::pcre2::regex& re{opt_re->get()};
  auto group_names{kphp::regex::details::collect_group_names(re)};

  const auto& regex_state{RegexInstanceState::get()};
  kphp::log::assertion(regex_state.match_context != nullptr);

  auto expected_opt_match_view{kphp::pcre2::matcher{re,
                                                    {regex_info.subject.c_str(), regex_info.subject.size()},
                                                    static_cast<size_t>(offset),
                                                    regex_state.match_context.get(),
                                                    *regex_state.regex_pcre2_match_data.get(),
                                                    regex_info.match_options}
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
      inner_ref = to_mixed_array({match_view, group_names, re.capture_count(), re.name_count(), kphp::regex::details::trailing_unmatch::skip, is_offset_capture,
                                  is_unmatched_as_null});
      return 0;
    });
  }
  return opt_match_view.has_value() ? 1 : 0;
}

Optional<int64_t> f$preg_match_all(const string& pattern, const string& subject,
                                   Optional<std::variant<std::monostate, std::reference_wrapper<mixed>>> opt_matches, int64_t flags, int64_t offset) noexcept {
  int64_t entire_match_count{};
  kphp::regex::details::Info regex_info{pattern, subject, {}};

  if (!kphp::regex::details::valid_regex_flags(flags, kphp::regex::PREG_NO_FLAGS, kphp::regex::PREG_PATTERN_ORDER, kphp::regex::PREG_SET_ORDER,
                                               kphp::regex::PREG_OFFSET_CAPTURE, kphp::regex::PREG_UNMATCHED_AS_NULL)) [[unlikely]] {
    return false;
  }
  if (!correct_offset(offset, {regex_info.subject.c_str(), regex_info.subject.size()})) [[unlikely]] {
    return false;
  }
  auto opt_re{kphp::regex::details::compile_regex(regex_info)};
  if (!opt_re.has_value()) [[unlikely]] {
    return false;
  }
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
    for (const auto* group_name : group_names) {
      if (group_name != nullptr) {
        inner_ref.set_value(string{group_name}, init_val);
      }
      inner_ref.push_back(init_val);
    }
  }

  const auto& regex_state{RegexInstanceState::get()};
  kphp::log::assertion(regex_state.match_context != nullptr);

  kphp::pcre2::matcher pcre2_matcher{re,
                                     {regex_info.subject.c_str(), regex_info.subject.size()},
                                     static_cast<size_t>(offset),
                                     regex_state.match_context.get(),
                                     *regex_state.regex_pcre2_match_data.get(),
                                     regex_info.match_options};

  auto expected_opt_match_view{pcre2_matcher.next()};
  while (expected_opt_match_view.has_value() && expected_opt_match_view->has_value()) {
    kphp::pcre2::match_view match_view{**expected_opt_match_view};
    set_all_matches(re, group_names, match_view, flags, matches);
    ++entire_match_count;
    expected_opt_match_view = pcre2_matcher.next();
  }
  if (!expected_opt_match_view.has_value()) [[unlikely]] {
    kphp::log::warning("can't find all matches due to match error: {}", expected_opt_match_view.error());
    return false;
  }

  return entire_match_count;
}

Optional<string> f$preg_replace(const string& pattern, const string& replacement, const string& subject, int64_t limit,
                                Optional<std::variant<std::monostate, std::reference_wrapper<int64_t>>> opt_count) noexcept {
  int64_t count{};
  vk::final_action count_finalizer{[&count, &opt_count]() noexcept {
    if (opt_count.has_value()) {
      kphp::log::assertion(std::holds_alternative<std::reference_wrapper<int64_t>>(opt_count.val()));
      auto& inner_ref{std::get<std::reference_wrapper<int64_t>>(opt_count.val()).get()};
      inner_ref = count;
    }
  }};

  if (limit < 0 && limit != kphp::regex::PREG_NOLIMIT) [[unlikely]] {
    kphp::log::warning("invalid limit {} in preg_replace", limit);
    return {};
  }

  // we need to replace PHP's back references with PCRE2 ones
  auto parser{preg_replacement_parser{{replacement.c_str(), replacement.size()}}};
  kphp::stl::string<kphp::memory::script_allocator> pcre2_replacement{};
  for (const auto& term : parser) {
    if (std::holds_alternative<char>(term)) {
      auto c{std::get<char>(term)};
      pcre2_replacement.push_back(c);
      if (c == '$') {
        pcre2_replacement.push_back('$');
      }
    } else {
      auto backreference{std::get<backref>(term)};
      pcre2_replacement.reserve(pcre2_replacement.size() + backreference.size() + 3);
      pcre2_replacement.append("${");
      pcre2_replacement.append(backreference);
      pcre2_replacement.append("}");
    }
  }

  kphp::regex::details::Info regex_info{pattern, subject, {pcre2_replacement.c_str(), pcre2_replacement.size()}};

  auto opt_re{kphp::regex::details::compile_regex(regex_info)};
  if (!opt_re.has_value()) [[unlikely]] {
    return {};
  }
  const auto& re{opt_re->get()};
  auto opt_replace_result{
      replace_regex(regex_info, re, limit == kphp::regex::PREG_NOLIMIT ? std::numeric_limits<uint64_t>::max() : static_cast<uint64_t>(limit))};
  if (!opt_replace_result.has_value()) {
    return {};
  }
  count = regex_info.replace_count;
  return std::move(*opt_replace_result);
}

Optional<string> f$preg_replace(const mixed& pattern, const string& replacement, const string& subject, int64_t limit,
                                Optional<std::variant<std::monostate, std::reference_wrapper<int64_t>>> opt_count) noexcept {
  int64_t count{};
  vk::final_action count_finalizer{[&count, &opt_count]() noexcept {
    if (opt_count.has_value()) {
      kphp::log::assertion(std::holds_alternative<std::reference_wrapper<int64_t>>(opt_count.val()));
      auto& inner_ref{std::get<std::reference_wrapper<int64_t>>(opt_count.val()).get()};
      inner_ref = count;
    }
  }};

  if (!kphp::regex::details::valid_preg_replace_mixed(pattern)) [[unlikely]] {
    return {};
  }

  if (pattern.is_string()) {
    return f$preg_replace(pattern.as_string(), replacement, subject, limit, count);
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
  vk::final_action count_finalizer{[&count, &opt_count]() noexcept {
    if (opt_count.has_value()) {
      kphp::log::assertion(std::holds_alternative<std::reference_wrapper<int64_t>>(opt_count.val()));
      auto& inner_ref{std::get<std::reference_wrapper<int64_t>>(opt_count.val()).get()};
      inner_ref = count;
    }
  }};

  if (!kphp::regex::details::valid_preg_replace_mixed(pattern) || !kphp::regex::details::valid_preg_replace_mixed(replacement)) [[unlikely]] {
    return {};
  }

  if (replacement.is_string()) {
    return f$preg_replace(pattern, replacement.as_string(), subject, limit, count);
  }
  if (pattern.is_string()) [[unlikely]] {
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
  vk::final_action count_finalizer{[&count, &opt_count]() noexcept {
    if (opt_count.has_value()) {
      kphp::log::assertion(std::holds_alternative<std::reference_wrapper<int64_t>>(opt_count.val()));
      auto& inner_ref{std::get<std::reference_wrapper<int64_t>>(opt_count.val()).get()};
      inner_ref = count;
    }
  }};

  if (!kphp::regex::details::valid_preg_replace_mixed(pattern) || !kphp::regex::details::valid_preg_replace_mixed(replacement) ||
      !kphp::regex::details::valid_preg_replace_mixed(subject)) [[unlikely]] {
    return {};
  }

  if (subject.is_string()) {
    return f$preg_replace(pattern, replacement, subject.as_string(), limit, count);
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

Optional<array<mixed>> f$preg_split(const string& pattern, const string& subject, int64_t limit, int64_t flags) noexcept {
  kphp::regex::details::Info regex_info{pattern, subject, {}};

  if (!kphp::regex::details::valid_regex_flags(flags, kphp::regex::PREG_NO_FLAGS, kphp::regex::PREG_SPLIT_NO_EMPTY, kphp::regex::PREG_SPLIT_DELIM_CAPTURE,
                                               kphp::regex::PREG_SPLIT_OFFSET_CAPTURE)) {
    return false;
  }
  auto opt_re{kphp::regex::details::compile_regex(regex_info)};
  if (!opt_re.has_value()) [[unlikely]] {
    return {};
  }
  const auto& re{opt_re->get()};
  auto opt_output{split_regex(regex_info, re, limit, (flags & kphp::regex::PREG_SPLIT_NO_EMPTY) != 0, //
                              (flags & kphp::regex::PREG_SPLIT_DELIM_CAPTURE) != 0,                   //
                              (flags & kphp::regex::PREG_SPLIT_OFFSET_CAPTURE) != 0)};
  if (!opt_output.has_value()) [[unlikely]] {
    return false;
  }

  return std::move(*opt_output);
}
