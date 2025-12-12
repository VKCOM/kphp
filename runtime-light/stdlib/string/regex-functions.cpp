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

int64_t skip_utf8_subsequent_bytes(size_t offset, const std::string_view subject) noexcept {
  // all multibyte utf8 runes consist of subsequent bytes,
  // these subsequent bytes start with 10 bit pattern
  // 0xc0 selects the two most significant bits, then we compare it to 0x80 (0b10000000)
  while (offset < subject.size() && ((static_cast<unsigned char>(subject[offset])) & 0xc0) == 0x80) {
    offset++;
  }
  return offset;
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

// returns the ending offset of the entire match
// *** importrant ***
// in case of a pattern order all_matches must already contain all groups as empty arrays before the first call to set_all_matches
PCRE2_SIZE set_all_matches(const kphp::regex::Info& regex_info, int64_t flags, std::optional<std::reference_wrapper<mixed>> opt_all_matches) noexcept {
  const auto is_pattern_order{!static_cast<bool>(flags & kphp::regex::PREG_SET_ORDER)};
  const auto is_offset_capture{static_cast<bool>(flags & kphp::regex::PREG_OFFSET_CAPTURE)};
  const auto is_unmatched_as_null{static_cast<bool>(flags & kphp::regex::PREG_UNMATCHED_AS_NULL)};

  if (regex_info.match_count <= 0) [[unlikely]] {
    return PCRE2_UNSET;
  }

  const auto& regex_state{RegexInstanceState::get()};

  // get the ouput vector from the match data
  const auto* ovector{pcre2_get_ovector_pointer_8(regex_state.regex_pcre2_match_data.get())};
  kphp::regex::details::match_view match_view{regex_info.subject, ovector, static_cast<size_t>(regex_info.match_count)};
  auto opt_entire_pattern_match{match_view.get_group(0)};
  if (!opt_entire_pattern_match.has_value()) [[unlikely]] {
    return PCRE2_UNSET;
  }
  auto entire_pattern_match_string_view{*opt_entire_pattern_match};
  const auto match_start_offset{std::distance(regex_info.subject.data(), entire_pattern_match_string_view.data())};
  const auto match_end_offset{match_start_offset + entire_pattern_match_string_view.size()};

  // early return in case we don't actually need to set matches
  if (!opt_all_matches.has_value()) {
    return match_end_offset;
  }

  auto last_unmatched_policy{is_pattern_order ? kphp::regex::details::trailing_unmatch::include : kphp::regex::details::trailing_unmatch::skip};
  mixed matches;
  if (is_offset_capture) {
    if (is_unmatched_as_null) {
      auto opt_dumped_matches{kphp::regex::dump_matches<true, true>(regex_info, match_view, last_unmatched_policy)};
      if (!opt_dumped_matches.has_value()) [[unlikely]] {
        return PCRE2_UNSET;
      }
      matches = std::move(*opt_dumped_matches);
    } else {
      auto opt_dumped_matches{kphp::regex::dump_matches<true, false>(regex_info, match_view, last_unmatched_policy)};
      if (!opt_dumped_matches.has_value()) [[unlikely]] {
        return PCRE2_UNSET;
      }
      matches = std::move(*opt_dumped_matches);
    }
  } else {
    if (is_unmatched_as_null) {
      auto opt_dumped_matches{kphp::regex::dump_matches<false, true>(regex_info, match_view, last_unmatched_policy)};
      if (!opt_dumped_matches.has_value()) [[unlikely]] {
        return PCRE2_UNSET;
      }
      matches = std::move(*opt_dumped_matches);
    } else {
      auto opt_dumped_matches{kphp::regex::dump_matches<false, false>(regex_info, match_view, last_unmatched_policy)};
      if (!opt_dumped_matches.has_value()) [[unlikely]] {
        return PCRE2_UNSET;
      }
      matches = std::move(*opt_dumped_matches);
    }
  }

  mixed& all_matches{(*opt_all_matches).get()};
  if (is_pattern_order) [[likely]] {
    for (const auto& it : std::as_const(matches)) {
      all_matches[it.get_key()].push_back(it.get_value());
    }
  } else {
    all_matches.push_back(matches);
  }

  return match_end_offset;
}

bool replace_regex(kphp::regex::Info& regex_info, uint64_t limit) noexcept {
  regex_info.replace_count = 0;
  if (regex_info.regex_code == nullptr) [[unlikely]] {
    return false;
  }

  const auto& regex_state{RegexInstanceState::get()};
  auto& runtime_ctx{RuntimeContext::get()};
  if (!regex_state.match_context) [[unlikely]] {
    return false;
  }

  const PCRE2_SIZE buffer_length{std::max({static_cast<string::size_type>(regex_info.subject.size()),
                                           static_cast<string::size_type>(RegexInstanceState::REPLACE_BUFFER_SIZE), runtime_ctx.static_SB.size()})};
  runtime_ctx.static_SB.clean().reserve(buffer_length);
  PCRE2_SIZE output_length{buffer_length};

  // replace all occurences
  if (limit == std::numeric_limits<uint64_t>::max()) [[likely]] {
    regex_info.replace_count = pcre2_substitute_8(regex_info.regex_code, reinterpret_cast<PCRE2_SPTR8>(regex_info.subject.data()), regex_info.subject.size(), 0,
                                                  regex_info.replace_options | PCRE2_SUBSTITUTE_GLOBAL, nullptr, regex_state.match_context.get(),
                                                  reinterpret_cast<PCRE2_SPTR8>(regex_info.replacement.data()), regex_info.replacement.size(),
                                                  reinterpret_cast<PCRE2_UCHAR8*>(runtime_ctx.static_SB.buffer()), std::addressof(output_length));

    if (regex_info.replace_count < 0) [[unlikely]] {
      kphp::log::warning("pcre2_substitute error: {}", kphp::regex::details::pcre2_error{.code = static_cast<int32_t>(regex_info.replace_count)});
      return false;
    }
  } else { // replace only 'limit' times
    size_t substitute_offset{};
    int64_t replacement_diff_acc{};
    PCRE2_SIZE length_after_replace{buffer_length};
    string str_after_replace{regex_info.subject.data(), static_cast<string::size_type>(regex_info.subject.size())};

    kphp::regex::matcher pcre2_matcher{regex_info, {}};
    for (; regex_info.replace_count < limit; ++regex_info.replace_count) {
      auto expected_opt_match_view{pcre2_matcher.next()};
      if (!expected_opt_match_view.has_value()) [[unlikely]] {
        kphp::log::warning("can't replace by pcre2 regex due to match error: {}", expected_opt_match_view.error());
        return false;
      }
      auto opt_match_view{*expected_opt_match_view};
      if (!opt_match_view.has_value()) {
        break;
      }

      auto match_view{*opt_match_view};
      auto opt_entire_pattern_match{match_view.get_group(0)};
      if (!opt_entire_pattern_match.has_value()) [[unlikely]] {
        return false;
      }
      auto entire_pattern_match_string_view{*opt_entire_pattern_match};
      const auto match_start_offset{std::distance(regex_info.subject.data(), entire_pattern_match_string_view.data())};
      const auto match_end_offset{match_start_offset + entire_pattern_match_string_view.size()};

      length_after_replace = buffer_length;
      if (auto replace_one_ret_code{pcre2_substitute_8(
              regex_info.regex_code, reinterpret_cast<PCRE2_SPTR8>(str_after_replace.c_str()), str_after_replace.size(), substitute_offset,
              regex_info.replace_options, nullptr, regex_state.match_context.get(), reinterpret_cast<PCRE2_SPTR8>(regex_info.replacement.data()),
              regex_info.replacement.size(), reinterpret_cast<PCRE2_UCHAR8*>(runtime_ctx.static_SB.buffer()), std::addressof(length_after_replace))};
          replace_one_ret_code != 1) [[unlikely]] {
        kphp::log::warning("pcre2_substitute error {}", replace_one_ret_code);
        return false;
      }

      replacement_diff_acc += regex_info.replacement.size() - (match_end_offset - match_start_offset);
      substitute_offset = match_end_offset + replacement_diff_acc;
      str_after_replace = {runtime_ctx.static_SB.buffer(), static_cast<string::size_type>(length_after_replace)};
    }

    output_length = length_after_replace;
  }

  if (regex_info.replace_count > 0) {
    runtime_ctx.static_SB.set_pos(output_length);
    regex_info.opt_replace_result.emplace(runtime_ctx.static_SB.str());
  }

  return true;
}

std::optional<array<mixed>> split_regex(kphp::regex::Info& regex_info, int64_t limit, bool no_empty, bool delim_capture, bool offset_capture) noexcept {
  if (limit == 0) {
    limit = kphp::regex::PREG_NOLIMIT;
  }

  const auto& regex_state{RegexInstanceState::get()};
  if (!regex_state.match_context) [[unlikely]] {
    return std::nullopt;
  }

  array<mixed> output{};

  kphp::regex::matcher pcre2_matcher{regex_info, {}};
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

    kphp::regex::details::match_view match_view{*opt_match_view};

    auto opt_entire_pattern_match{match_view.get_group(0)};
    if (!opt_entire_pattern_match.has_value()) [[unlikely]] {
      return std::nullopt;
    }
    auto entire_pattern_match_string_view{*opt_entire_pattern_match};

    if (const auto size{std::distance(regex_info.subject.data(), entire_pattern_match_string_view.data()) - offset}; !no_empty || size != 0) {
      string val{std::next(regex_info.subject.data(), offset), static_cast<string::size_type>(size)};

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
                                                           return static_cast<int64_t>(std::distance(regex_info.subject.data(), submatch_string_view.data()));
                                                         })
                                                         .value_or(-1));
          } else {
            output_val = std::move(val);
          }

          output.emplace_back(std::move(output_val));
        }
      }
    }

    offset = std::distance(regex_info.subject.data(), entire_pattern_match_string_view.data()) + entire_pattern_match_string_view.size();
  }

  const auto size{regex_info.subject.size() - offset};
  if (!no_empty || size != 0) {
    string val{std::next(regex_info.subject.data(), offset), static_cast<string::size_type>(size)};

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

void count_updater::operator()() const noexcept {
  if (!opt_count.has_value()) {
    return;
  }
  kphp::log::assertion(std::holds_alternative<std::reference_wrapper<int64_t>>(opt_count.val()));
  auto& inner_ref{std::get<std::reference_wrapper<int64_t>>(opt_count.val()).get()};
  inner_ref = count;
}

std::optional<std::string_view> match_view::get_group(size_t i) const noexcept {
  if (i >= m_num_groups) {
    return std::nullopt;
  }

  kphp::log::assertion(m_ovector_ptr);
  // ovector is an array of offset pairs
  PCRE2_SIZE start{m_ovector_ptr[2 * i]};
  PCRE2_SIZE end{m_ovector_ptr[(2 * i) + 1]};

  if (start == PCRE2_UNSET) {
    return std::nullopt;
  }

  return m_subject_data.substr(start, end - start);
}

std::pair<string_buffer&, const PCRE2_SIZE> reserve_buffer(std::string_view subject) noexcept {
  auto& runtime_ctx{RuntimeContext::get()};

  const PCRE2_SIZE buffer_length{std::max(
      {static_cast<string::size_type>(subject.size()), static_cast<string::size_type>(RegexInstanceState::REPLACE_BUFFER_SIZE), runtime_ctx.static_SB.size()})};
  runtime_ctx.static_SB.clean().reserve(buffer_length);
  return {runtime_ctx.static_SB, buffer_length};
}

} // namespace details

matcher::matcher(const Info& info, size_t match_from) noexcept
    : m_regex_info{info},
      m_match_options{info.match_options},
      m_current_offset{match_from} {
  kphp::log::assertion(info.regex_code != nullptr);

  const auto& regex_state{RegexInstanceState::get()};
  m_match_data = regex_state.regex_pcre2_match_data.get();
  kphp::log::assertion(m_match_data);
}

std::expected<std::optional<details::match_view>, details::pcre2_error> matcher::next() noexcept {
  const auto& regex_state{RegexInstanceState::get()};
  kphp::log::assertion(m_regex_info.regex_code != nullptr && regex_state.match_context);

  const auto* const ovector{pcre2_get_ovector_pointer_8(m_match_data)};

  while (true) {
    // Try to find match
    int32_t ret_code{pcre2_match_8(m_regex_info.regex_code, reinterpret_cast<PCRE2_SPTR8>(m_regex_info.subject.data()), m_regex_info.subject.size(),
                                   m_current_offset, m_match_options, m_match_data, regex_state.match_context.get())};
    // From https://www.pcre.org/current/doc/html/pcre2_match.html
    // The return from pcre2_match() is one more than the highest numbered capturing pair that has been set
    // (for example, 1 if there are no captures), zero if the vector of offsets is too small, or a negative error code for no match and other errors.
    if (ret_code < 0 && ret_code != PCRE2_ERROR_NOMATCH) [[unlikely]] {
      return std::unexpected{details::pcre2_error{.code = ret_code}};
    }
    size_t match_count{ret_code != PCRE2_ERROR_NOMATCH ? static_cast<size_t>(ret_code) : 0};

    if (match_count == 0) {
      // If match is not found
      if (m_match_options == m_regex_info.match_options || m_current_offset == m_regex_info.subject.size()) {
        // Here we are sure that there are no more matches here
        return std::nullopt;
      }
      // Here we know that we were looking for a non-empty and anchored match,
      // and we're going to try searching from the next character with the default options.
      ++m_current_offset;
      m_current_offset =
          static_cast<bool>(m_regex_info.compile_options & PCRE2_UTF) ? skip_utf8_subsequent_bytes(m_current_offset, m_regex_info.subject) : m_current_offset;
      m_match_options = m_regex_info.match_options;
      continue;
    }

    // Match found
    PCRE2_SIZE match_start{ovector[0]};
    PCRE2_SIZE match_end{ovector[1]};

    m_current_offset = match_end;
    if (match_end == match_start) {
      // If an empty match is found, try searching for a non-empty attached match next time.
      m_match_options |= PCRE2_NOTEMPTY_ATSTART | PCRE2_ANCHORED;
    } else {
      // Else use default options
      m_match_options = m_regex_info.match_options;
    }
    return details::match_view{m_regex_info.subject, ovector, match_count};
  }
}

bool compile_regex(Info& regex_info) noexcept {
  const vk::final_action finalizer{[&regex_info]() noexcept {
    if (regex_info.regex_code != nullptr) [[likely]] {
      pcre2_pattern_info_8(regex_info.regex_code, PCRE2_INFO_CAPTURECOUNT, std::addressof(regex_info.capture_count));
      ++regex_info.capture_count; // to also count entire match
    } else {
      regex_info.capture_count = 0;
    }
  }};

  auto& regex_state{RegexInstanceState::get()};
  if (!regex_state.compile_context) [[unlikely]] {
    return false;
  }

  // check runtime cache
  if (auto opt_ref{regex_state.get_compiled_regex(regex_info.regex)}; opt_ref.has_value()) {
    const auto& [compile_options, regex_code]{opt_ref->get()};
    regex_info.compile_options = compile_options;
    regex_info.regex_code = regex_code.get();
    return true;
  }

  if (regex_info.regex.empty()) {
    kphp::log::warning("empty regex");
    return false;
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
    return false;
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
    return false;
  }
  // UTF-8 validation
  if (static_cast<bool>(compile_options & PCRE2_UTF)) {
    if (!mb_UTF8_check(regex_info.regex.c_str())) [[unlikely]] {
      kphp::log::warning("invalid UTF-8 pattern: {}", regex_info.regex.c_str());
      return false;
    }
    if (!mb_UTF8_check(regex_info.subject.data())) [[unlikely]] {
      kphp::log::warning("invalid UTF-8 subject: {}", regex_info.subject);
      return false;
    }
  }

  // remove the end delimiter
  regex_body.remove_suffix(1);
  regex_info.compile_options = compile_options;

  // compile pcre2_code
  int32_t error_number{};
  PCRE2_SIZE error_offset{};
  regex_pcre2_code_t regex_code{pcre2_compile_8(reinterpret_cast<PCRE2_SPTR8>(regex_body.data()), regex_body.size(), regex_info.compile_options,
                                                std::addressof(error_number), std::addressof(error_offset), regex_state.compile_context.get()),
                                pcre2_code_free_8};
  if (!regex_code) [[unlikely]] {
    kphp::log::warning("can't compile pcre2 regex due to error at offset {}: {}", error_offset, kphp::regex::details::pcre2_error{.code = error_number});
    return false;
  }

  regex_info.regex_code = regex_code.get();
  // add compiled code to runtime cache
  regex_state.add_compiled_regex(regex_info.regex, compile_options, std::move(regex_code));

  return true;
}

bool collect_group_names(Info& regex_info) noexcept {
  if (regex_info.regex_code == nullptr) [[unlikely]] {
    return false;
  }

  // initialize an array of strings to hold group names
  regex_info.group_names.resize(regex_info.capture_count);

  uint32_t name_count{};
  pcre2_pattern_info_8(regex_info.regex_code, PCRE2_INFO_NAMECOUNT, std::addressof(name_count));
  if (name_count == 0) {
    return true;
  }

  PCRE2_SPTR8 name_table{};
  uint32_t name_entry_size{};
  pcre2_pattern_info_8(regex_info.regex_code, PCRE2_INFO_NAMETABLE, std::addressof(name_table));
  pcre2_pattern_info_8(regex_info.regex_code, PCRE2_INFO_NAMEENTRYSIZE, std::addressof(name_entry_size));

  PCRE2_SPTR8 entry{name_table};
  for (auto i{0}; i < name_count; ++i) {
    const auto group_number{static_cast<uint16_t>((entry[0] << 8) | entry[1])};
    PCRE2_SPTR8 group_name{std::next(entry, 2)};
    regex_info.group_names[group_number] = reinterpret_cast<const char*>(group_name);
    std::advance(entry, name_entry_size);
  }

  return true;
}

std::optional<string> replace_one(const Info& info, std::string_view subject, std::string_view replacement, string_buffer& sb, size_t buffer_length,
                                  size_t substitute_offset) noexcept {
  const auto& regex_state{RegexInstanceState::get()};
  if (auto replace_one_ret_code{pcre2_substitute_8(info.regex_code, reinterpret_cast<PCRE2_SPTR8>(subject.data()), subject.size(), substitute_offset,
                                                   info.replace_options, nullptr, regex_state.match_context.get(),
                                                   reinterpret_cast<PCRE2_SPTR8>(replacement.data()), replacement.size(),
                                                   reinterpret_cast<PCRE2_UCHAR8*>(sb.buffer()), std::addressof(buffer_length))};
      replace_one_ret_code != 1) [[unlikely]] {
    kphp::log::warning("pcre2_substitute error: {}", details::pcre2_error{.code = replace_one_ret_code});
    return std::nullopt;
  }
  return string{sb.buffer(), static_cast<string::size_type>(buffer_length)};
}

} // namespace kphp::regex

Optional<int64_t> f$preg_match(const string& pattern, const string& subject, Optional<std::variant<std::monostate, std::reference_wrapper<mixed>>> opt_matches,
                               int64_t flags, int64_t offset) noexcept {
  kphp::regex::Info regex_info{pattern, {subject.c_str(), subject.size()}, {}};

  if (!kphp::regex::valid_regex_flags(flags, kphp::regex::PREG_NO_FLAGS, kphp::regex::PREG_OFFSET_CAPTURE, kphp::regex::PREG_UNMATCHED_AS_NULL)) [[unlikely]] {
    return false;
  }
  if (!correct_offset(offset, regex_info.subject)) [[unlikely]] {
    return false;
  }
  if (!compile_regex(regex_info)) [[unlikely]] {
    return false;
  }
  if (!collect_group_names(regex_info)) [[unlikely]] {
    return false;
  }

  const auto& regex_state{RegexInstanceState::get()};
  kphp::log::assertion(regex_info.regex_code != nullptr && regex_state.match_context);

  auto expected_opt_match_view{kphp::regex::matcher{regex_info, static_cast<size_t>(offset)}.next()};
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
    opt_match_view.transform([is_offset_capture, is_unmatched_as_null, &inner_ref, &regex_info](const auto& match_view) {
      if (is_offset_capture) {
        if (is_unmatched_as_null) {
          auto opt_dumped_matches{kphp::regex::dump_matches<true, true>(regex_info, match_view, kphp::regex::details::trailing_unmatch::skip)};
          if (opt_dumped_matches.has_value()) [[likely]] {
            inner_ref = std::move(*opt_dumped_matches);
          }
        } else {
          auto opt_dumped_matches{kphp::regex::dump_matches<true, false>(regex_info, match_view, kphp::regex::details::trailing_unmatch::skip)};
          if (opt_dumped_matches.has_value()) [[likely]] {
            inner_ref = std::move(*opt_dumped_matches);
          }
        }
      } else {
        if (is_unmatched_as_null) {
          auto opt_dumped_matches{kphp::regex::dump_matches<false, true>(regex_info, match_view, kphp::regex::details::trailing_unmatch::skip)};
          if (opt_dumped_matches.has_value()) [[likely]] {
            inner_ref = std::move(*opt_dumped_matches);
          }
        } else {
          auto opt_dumped_matches{kphp::regex::dump_matches<false, false>(regex_info, match_view, kphp::regex::details::trailing_unmatch::skip)};
          if (opt_dumped_matches.has_value()) [[likely]] {
            inner_ref = std::move(*opt_dumped_matches);
          }
        }
      }
      return 0;
    });
  }
  return opt_match_view.has_value() ? 1 : 0;
}

Optional<int64_t> f$preg_match_all(const string& pattern, const string& subject,
                                   Optional<std::variant<std::monostate, std::reference_wrapper<mixed>>> opt_matches, int64_t flags, int64_t offset) noexcept {
  int64_t entire_match_count{};
  kphp::regex::Info regex_info{pattern, {subject.c_str(), subject.size()}, {}};

  if (!kphp::regex::valid_regex_flags(flags, kphp::regex::PREG_NO_FLAGS, kphp::regex::PREG_PATTERN_ORDER, kphp::regex::PREG_SET_ORDER,
                                      kphp::regex::PREG_OFFSET_CAPTURE, kphp::regex::PREG_UNMATCHED_AS_NULL)) [[unlikely]] {
    return false;
  }
  if (!correct_offset(offset, regex_info.subject)) [[unlikely]] {
    return false;
  }
  if (!compile_regex(regex_info)) [[unlikely]] {
    return false;
  }
  if (!collect_group_names(regex_info)) [[unlikely]] {
    return false;
  }

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
    for (const auto* group_name : regex_info.group_names) {
      if (group_name != nullptr) {
        inner_ref.set_value(string{group_name}, init_val);
      }
      inner_ref.push_back(init_val);
    }
  }

  kphp::regex::matcher pcre2_matcher{regex_info, static_cast<size_t>(offset)};

  auto expected_opt_match_view{pcre2_matcher.next()};
  while (expected_opt_match_view.has_value() && expected_opt_match_view->has_value()) {
    kphp::regex::details::match_view match_view{**expected_opt_match_view};
    regex_info.match_count = match_view.size();
    set_all_matches(regex_info, flags, matches);
    if (regex_info.match_count > 0) {
      ++entire_match_count;
    }
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
  auto cf = kphp::regex::count_finalizer{opt_count};

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

  kphp::regex::Info regex_info{pattern, {subject.c_str(), subject.size()}, {pcre2_replacement.c_str(), pcre2_replacement.size()}};

  if (!compile_regex(regex_info)) [[unlikely]] {
    return {};
  }
  if (!replace_regex(regex_info, limit == kphp::regex::PREG_NOLIMIT ? std::numeric_limits<uint64_t>::max() : static_cast<uint64_t>(limit))) {
    return {};
  }
  cf.count = regex_info.replace_count;
  return regex_info.opt_replace_result.value_or(subject);
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

  if (!regex_impl_::valid_preg_replace_mixed(pattern)) [[unlikely]] {
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

  if (!regex_impl_::valid_preg_replace_mixed(pattern) || !regex_impl_::valid_preg_replace_mixed(replacement)) [[unlikely]] {
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

  if (!regex_impl_::valid_preg_replace_mixed(pattern) || !regex_impl_::valid_preg_replace_mixed(replacement) || !regex_impl_::valid_preg_replace_mixed(subject))
      [[unlikely]] {
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
  kphp::regex::Info regex_info{pattern, {subject.c_str(), subject.size()}, {}};

  if (!kphp::regex::valid_regex_flags(flags, kphp::regex::PREG_NO_FLAGS, kphp::regex::PREG_SPLIT_NO_EMPTY, kphp::regex::PREG_SPLIT_DELIM_CAPTURE,
                                      kphp::regex::PREG_SPLIT_OFFSET_CAPTURE)) {
    return false;
  }
  if (!compile_regex(regex_info)) [[unlikely]] {
    return false;
  }
  auto opt_output{split_regex(regex_info, limit, (flags & kphp::regex::PREG_SPLIT_NO_EMPTY) != 0, //
                              (flags & kphp::regex::PREG_SPLIT_DELIM_CAPTURE) != 0,               //
                              (flags & kphp::regex::PREG_SPLIT_OFFSET_CAPTURE) != 0)};
  if (!opt_output.has_value()) [[unlikely]] {
    return false;
  }

  return std::move(*opt_output);
}
