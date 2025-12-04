// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime-light/stdlib/string/regex-functions.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <concepts>
#include <cstddef>
#include <cstdint>
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
#include "runtime-light/stdlib/diagnostics/logs.h"
#include "runtime-light/stdlib/string/pcre2-functions.h"
#include "runtime-light/stdlib/string/regex-include.h"
#include "runtime-light/stdlib/string/regex-state.h"

namespace {

struct backref {
  std::string_view digits;
};

template<typename... Args>
requires((std::is_same_v<Args, int64_t> && ...) && sizeof...(Args) > 0)
bool valid_regex_flags(int64_t flags, Args... supported_flags) noexcept {
  const bool valid{(flags & ~(supported_flags | ...)) == kphp::regex::PREG_NO_FLAGS};
  if (!valid) [[unlikely]] {
    kphp::log::warning("invalid flags: {}", flags);
  }
  return valid;
}

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

int64_t skip_utf8_subsequent_bytes(int64_t offset, const std::string_view subject) noexcept {
  // all multibyte utf8 runes consist of subsequent bytes,
  // these subsequent bytes start with 10 bit pattern
  // 0xc0 selects the two most significant bits, then we compare it to 0x80 (0b10000000)
  while (offset < static_cast<int64_t>(subject.size()) && ((static_cast<unsigned char>(subject[offset])) & 0xc0) == 0x80) {
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
              auto digits_end_pos = 1 + br.digits.size();
              if (digits_end_pos < preg_replacement.size() && preg_replacement[digits_end_pos] == '}') {
                preg_replacement = preg_replacement.substr(1 + br.digits.size() + 1);
                return br;
              }

              return std::nullopt;
            })
            .value_or('$');
      }

      return try_get_backref(preg_replacement)
          .transform([this](auto br) noexcept -> replacement_term {
            auto digits_end_pos = br.digits.size();
            preg_replacement = preg_replacement.substr(digits_end_pos);
            return br;
          })
          .value_or('$');

    case '\\': {
      // \1
      auto back_reference_opt{try_get_backref(preg_replacement).transform([this](auto br) noexcept -> replacement_term {
        auto digits_end_pos = br.digits.size();
        preg_replacement = preg_replacement.substr(digits_end_pos);
        return br;
      })};
      if (back_reference_opt.has_value()) {
        return *back_reference_opt;
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
    iterator operator++(int) noexcept {
      iterator temp = *this;
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

class pcre2_iterator {
public:
  using value_type = kphp::pcre2::match_view;
  using difference_type = std::ptrdiff_t;
  using reference = value_type;
  using pointer = value_type*;
  using iterator_category = std::forward_iterator_tag;

  pcre2_iterator() noexcept
      : m_is_valid{true} {}

  pcre2_iterator(const kphp::regex::Info& info, const kphp::pcre2::compiled_regex& compiled_regex, size_t match_from) noexcept
      : m_regex_info{std::addressof(info)},
        m_compiled_regex{std::addressof(compiled_regex)},
        m_match_options{info.match_options},
        m_current_offset{match_from} {
    const auto& regex_state{RegexInstanceState::get()};
    m_match_data = regex_state.regex_pcre2_match_data.get();
    if (!m_match_data) {
      return;
    }

    if (!m_compiled_regex->validate(info.subject)) {
      return;
    }

    if (!m_compiled_regex->validate(info.subject)) {
      return;
    }

    m_is_valid = true;
    m_is_end = false;
  }

  bool is_terminal() const noexcept {
    return !m_is_valid || m_is_end;
  }

  bool is_valid() const noexcept {
    return m_is_valid;
  }

  reference operator*() const noexcept {
    return m_last_match_view.value_or(kphp::pcre2::match_view{m_regex_info->subject, pcre2_get_ovector_pointer_8(m_match_data), {}});
  }

  pcre2_iterator& operator++() noexcept {
    increment();
    return *this;
  }
  pcre2_iterator operator++(int) noexcept {
    pcre2_iterator temp{*this};
    increment();
    return temp;
  }

  bool operator==(const pcre2_iterator& other) const noexcept {
    return is_terminal() && other.is_terminal();
  }
  bool operator!=(const pcre2_iterator& other) const noexcept {
    return !(*this == other);
  }

private:
  void increment() noexcept {
    auto& ri{*m_regex_info};
    auto* const ovector{pcre2_get_ovector_pointer_8(m_match_data)};

    while (true) {
      // Try to find match
      m_last_match_view = m_compiled_regex->match(m_regex_info->subject, m_current_offset, m_match_options);
      if (!m_last_match_view.has_value()) {
        // std::nullopt means error
        m_is_end = true;
        m_is_valid = false;
        return;
      }

      auto& math_view{*m_last_match_view};

      if (math_view.size() == 0) {
        // If match is not found
        if (m_match_options == ri.match_options || m_current_offset == ri.subject.size()) {
          // Here we are sure that there are no more matches here
          m_is_end = true;
          return;
        }
        // Here we know that we were looking for a non-empty and anchored match,
        // and we're going to try searching from the next character with the default options.
        ++m_current_offset;
        m_current_offset = m_compiled_regex->is_utf() ? skip_utf8_subsequent_bytes(m_current_offset, ri.subject) : m_current_offset;
        m_match_options = ri.match_options;
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
        m_match_options = ri.match_options;
      }
      return;
    }
  }

  const kphp::regex::Info* const m_regex_info{nullptr};
  const kphp::pcre2::compiled_regex* m_compiled_regex{nullptr};
  uint64_t m_match_options;
  PCRE2_SIZE m_current_offset;
  pcre2_match_data_8* m_match_data{nullptr};
  std::optional<kphp::pcre2::match_view> m_last_match_view;
  bool m_is_end{true};
  bool m_is_valid{false};
};

// returns the ending offset of the entire match
// *** importrant ***
// in case of a pattern order all_matches must already contain all groups as empty arrays before the first call to set_all_matches
PCRE2_SIZE set_all_matches(const kphp::pcre2::compiled_regex& compiled_regex, const kphp::pcre2::group_names_t& group_names,
                           const kphp::pcre2::match_view& match_view, int64_t flags, std::optional<std::reference_wrapper<mixed>> opt_all_matches) noexcept {
  const auto is_pattern_order{!static_cast<bool>(flags & kphp::regex::PREG_SET_ORDER)};

  // early return in case we don't actually need to set matches
  if (!opt_all_matches.has_value()) {
    return set_matches(compiled_regex, group_names, match_view, flags, std::nullopt,
                       is_pattern_order ? kphp::regex::trailing_unmatch::include : kphp::regex::trailing_unmatch::skip);
  }

  mixed matches;
  PCRE2_SIZE offset{set_matches(compiled_regex, group_names, match_view, flags, matches,
                                is_pattern_order ? kphp::regex::trailing_unmatch::include : kphp::regex::trailing_unmatch::skip)};
  if (offset == PCRE2_UNSET) [[unlikely]] {
    return offset;
  }

  mixed& all_matches{(*opt_all_matches).get()};
  if (is_pattern_order) [[likely]] {
    for (auto& it : std::as_const(matches)) {
      all_matches[it.get_key()].push_back(it.get_value());
    }
  } else {
    all_matches.push_back(matches);
  }

  return offset;
}

std::optional<array<mixed>> split_regex(kphp::regex::Info& regex_info, const kphp::pcre2::compiled_regex& compiled_regex, int64_t limit_val, bool no_empty,
                                        bool delim_capture, bool offset_capture) noexcept {
  const char* offset{regex_info.subject.data()};

  if (limit_val == 0) {
    limit_val = kphp::regex::PREG_NOLIMIT;
  }

  const auto& regex_state{RegexInstanceState::get()};
  if (!regex_state.match_context) [[unlikely]] {
    return std::nullopt;
  }

  array<mixed> output{};

  pcre2_iterator it{regex_info, compiled_regex, {}};
  if (!it.is_valid()) {
    return std::nullopt;
  }

  pcre2_iterator end_it{};

  for (; it != end_it; ++it) {
    kphp::pcre2::match_view match_view{*it};

    auto entire_pattern_match_opt{match_view.get_group(0)};
    if (!entire_pattern_match_opt.has_value()) [[unlikely]] {
      return std::nullopt;
    }
    auto entire_pattern_match_sv{*entire_pattern_match_opt};

    if (!(limit_val == kphp::regex::PREG_NOLIMIT || limit_val > 1)) {
      break;
    }

    if (const auto size{entire_pattern_match_sv.data() - offset}; !no_empty || size != 0) {
      auto val{string{offset, static_cast<string::size_type>(size)}};

      mixed output_val;
      if (offset_capture) {
        output_val = array<mixed>::create(std::move(val), static_cast<int64_t>(offset - regex_info.subject.data()));
      } else {
        output_val = std::move(val);
      }

      output.push_back(std::move(output_val));
      if (limit_val != kphp::regex::PREG_NOLIMIT) {
        limit_val--;
      }
    }

    if (delim_capture) {
      for (auto i{1uz}; i < match_view.size(); i++) {
        auto sv_opt{match_view.get_group(i)};
        auto sv{sv_opt.value_or(std::string_view{})};
        const auto size{sv.size()};
        if (!no_empty || size != 0) {
          string val;
          if (sv_opt.has_value()) [[likely]] {
            val = string{sv.data(), static_cast<string::size_type>(size)};
          }

          mixed output_val;
          if (offset_capture) {
            output_val = array<mixed>::create(
                std::move(val),
                sv_opt.transform([&regex_info](auto sv) noexcept { return static_cast<int64_t>(sv.data() - regex_info.subject.data()); }).value_or(-1));
          } else {
            output_val = std::move(val);
          }

          output.push_back(std::forward<decltype(output_val)>(output_val));
        }
      }
    }

    offset = std::next(entire_pattern_match_sv.data(), entire_pattern_match_sv.size());
  }

  if (!it.is_valid()) [[unlikely]] {
    return std::nullopt;
  }

  const auto size{regex_info.subject.size() - (offset - regex_info.subject.data())};
  if (!no_empty || size != 0) {
    auto val{string{offset, static_cast<string::size_type>(size)}};

    mixed output_val;
    if (offset_capture) {
      output_val = array<mixed>::create(std::move(val), static_cast<int64_t>(offset - regex_info.subject.data()));
    } else {
      output_val = std::move(val);
    }

    output.push_back(std::forward<decltype(output_val)>(output_val));
  }

  return output;
}

} // namespace

namespace kphp::regex {

void count_updater::operator()() const noexcept {
  if (!opt_count.has_value()) {
    return;
  }
  kphp::log::assertion(std::holds_alternative<std::reference_wrapper<int64_t>>(opt_count.val()));
  auto& inner_ref{std::get<std::reference_wrapper<int64_t>>(opt_count.val()).get()};
  inner_ref = count;
}

// returns the ending offset of the entire match
PCRE2_SIZE set_matches(const kphp::pcre2::compiled_regex& compiled_regex, const kphp::pcre2::group_names_t& group_names, const pcre2::match_view& match_view,
                       int64_t flags, std::optional<std::reference_wrapper<mixed>> opt_matches, trailing_unmatch last_unmatched_policy) noexcept {
  if (match_view.size() <= 0) [[unlikely]] {
    return PCRE2_UNSET;
  }

  auto end_offset_opt{match_view.get_end_offset(0)};
  if (!end_offset_opt.has_value()) [[unlikely]] {
    return PCRE2_UNSET;
  }
  auto end_offset{*end_offset_opt};
  // early return in case we don't need to actually set matches
  if (!opt_matches.has_value()) {
    return end_offset;
  }

  const auto is_offset_capture{static_cast<bool>(flags & kphp::regex::PREG_OFFSET_CAPTURE)};
  const auto is_unmatched_as_null{static_cast<bool>(flags & kphp::regex::PREG_UNMATCHED_AS_NULL)};
  // calculate last matched group
  int64_t last_matched_group{-1};
  for (auto i{0}; i < match_view.size(); ++i) {
    if (match_view.get_start_offset(i).has_value()) {
      last_matched_group = i;
    }
  }
  // retrieve the named groups count
  uint32_t named_groups_count{compiled_regex.named_groups_count()};

  // reserve enough space for output
  array<mixed> output{array_size{static_cast<int64_t>(group_names.size() + named_groups_count), named_groups_count == 0}};
  for (auto i{0}; i < group_names.size(); ++i) {
    mixed output_val;
    if (i > last_matched_group) {
      // skip unmatched groups at the end unless unmatched_as_null is set
      if (last_unmatched_policy == trailing_unmatch::skip && !is_unmatched_as_null) [[unlikely]] {
        break;
      }

      mixed match_val;             // NULL value
      if (!is_unmatched_as_null) { // handle unmatched group
        match_val = string{};
      }

      if (is_offset_capture) {
        output_val = array<mixed>::create(std::move(match_val), int64_t{-1});
      } else {
        output_val = std::move(match_val);
      }
    } else {
      auto match_opt{match_view.get_group(i)};

      mixed match_val;             // NULL value
      if (match_opt.has_value()) { // handle matched group
        auto match{*match_opt};
        const auto match_size{match.size()};
        match_val = string{match.data(), static_cast<string::size_type>(match_size)};
      } else if (!is_unmatched_as_null) { // handle unmatched group
        match_val = string{};
      }

      if (is_offset_capture) {
        const auto match_start_offset_opt{match_view.get_start_offset(i)};
        output_val = array<mixed>::create(
            std::move(match_val),
            match_start_offset_opt.transform([](auto match_start_offset) { return static_cast<int64_t>(match_start_offset); }).value_or(-1));
      } else {
        output_val = std::move(match_val);
      }
    }

    if (group_names[i] != nullptr) {
      output.set_value(string{group_names[i]}, output_val);
    }
    output.push_back(output_val);
  }

  (*opt_matches).get() = std::move(output);
  return end_offset;
}

std::pair<string_buffer&, const size_t> reserve_buffer(std::string_view subject) noexcept {
  auto& runtime_ctx{RuntimeContext::get()};

  const PCRE2_SIZE buffer_length{std::max(
      {static_cast<string::size_type>(subject.size()), static_cast<string::size_type>(RegexInstanceState::REPLACE_BUFFER_SIZE), runtime_ctx.static_SB.size()})};
  runtime_ctx.static_SB.clean().reserve(buffer_length);
  return {runtime_ctx.static_SB, buffer_length};
}

std::optional<string> make_replace_result(int64_t replace_count, string_buffer& sb, size_t output_length) noexcept {
  if (replace_count > 0) {
    sb.set_pos(output_length);
    return sb.str();
  }

  return std::nullopt;
}

} // namespace kphp::regex

Optional<int64_t> f$preg_match(const string& pattern, const string& subject, Optional<std::variant<std::monostate, std::reference_wrapper<mixed>>> opt_matches,
                               int64_t flags, int64_t offset) noexcept {
  kphp::regex::Info regex_info{pattern, {subject.c_str(), subject.size()}, {}};

  if (!valid_regex_flags(flags, kphp::regex::PREG_NO_FLAGS, kphp::regex::PREG_OFFSET_CAPTURE, kphp::regex::PREG_UNMATCHED_AS_NULL)) [[unlikely]] {
    return false;
  }
  if (!correct_offset(offset, regex_info.subject)) [[unlikely]] {
    return false;
  }
  auto* compiled_regex{kphp::pcre2::compiled_regex::compile(pattern)};
  if (compiled_regex == nullptr) [[unlikely]] {
    return false;
  }
  auto group_names{compiled_regex->collect_group_names()};
  auto match_view_opt{compiled_regex->match(regex_info.subject, offset, regex_info.match_options)};
  if (!match_view_opt.has_value()) [[unlikely]] {
    return false;
  }
  auto& match_view{*match_view_opt};
  regex_info.match_count = match_view.size();

  std::optional<std::reference_wrapper<mixed>> matches{};
  if (opt_matches.has_value()) {
    kphp::log::assertion(std::holds_alternative<std::reference_wrapper<mixed>>(opt_matches.val()));
    auto& inner_ref{std::get<std::reference_wrapper<mixed>>(opt_matches.val()).get()};
    inner_ref = array<mixed>{};
    matches.emplace(inner_ref);
  }
  kphp::regex::set_matches(*compiled_regex, group_names, match_view, flags, matches, kphp::regex::trailing_unmatch::skip);
  return regex_info.match_count > 0 ? 1 : 0;
}

Optional<int64_t> f$preg_match_all(const string& pattern, const string& subject,
                                   Optional<std::variant<std::monostate, std::reference_wrapper<mixed>>> opt_matches, int64_t flags, int64_t offset) noexcept {
  int64_t entire_match_count{};
  kphp::regex::Info regex_info{pattern, {subject.c_str(), subject.size()}, {}};

  if (!valid_regex_flags(flags, kphp::regex::PREG_NO_FLAGS, kphp::regex::PREG_PATTERN_ORDER, kphp::regex::PREG_SET_ORDER, kphp::regex::PREG_OFFSET_CAPTURE,
                         kphp::regex::PREG_UNMATCHED_AS_NULL)) [[unlikely]] {
    return false;
  }
  if (!correct_offset(offset, regex_info.subject)) [[unlikely]] {
    return false;
  }
  auto* compiled_regex{kphp::pcre2::compiled_regex::compile(pattern)};
  if (compiled_regex == nullptr) [[unlikely]] {
    return false;
  }
  auto group_names{compiled_regex->collect_group_names()};

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

  pcre2_iterator it{regex_info, *compiled_regex, static_cast<size_t>(offset)};
  if (!it.is_valid()) {
    return false;
  }

  pcre2_iterator end_it{};

  for (; it != end_it; ++it) {
    kphp::pcre2::match_view match_view{*it};
    regex_info.match_count = match_view.size();
    set_all_matches(*compiled_regex, group_names, match_view, flags, matches);
    if (regex_info.match_count > 0) {
      ++entire_match_count;
    }
  }
  if (!it.is_valid()) [[unlikely]] {
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
      pcre2_replacement.reserve(pcre2_replacement.size() + backreference.digits.size() + 3);
      pcre2_replacement.append("${");
      pcre2_replacement.append(backreference.digits);
      pcre2_replacement.append("}");
    }
  }

  kphp::regex::Info regex_info{pattern, {subject.c_str(), subject.size()}, {pcre2_replacement.c_str(), pcre2_replacement.size()}};

  auto* compiled_regex{kphp::pcre2::compiled_regex::compile(pattern)};
  if (compiled_regex == nullptr) [[unlikely]] {
    return {};
  }
  regex_info.opt_replace_result = compiled_regex->replace(
      subject, regex_info.replace_options, regex_info.replacement, regex_info.match_options,
      limit == kphp::regex::PREG_NOLIMIT ? std::numeric_limits<uint64_t>::max() : static_cast<uint64_t>(limit), regex_info.replace_count);
  if (!regex_info.opt_replace_result.has_value()) [[unlikely]] {
    return {};
  }
  cf.count = regex_info.replace_count;
  return *regex_info.opt_replace_result;
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

  if (!valid_regex_flags(flags, kphp::regex::PREG_NO_FLAGS, kphp::regex::PREG_SPLIT_NO_EMPTY, kphp::regex::PREG_SPLIT_DELIM_CAPTURE,
                         kphp::regex::PREG_SPLIT_OFFSET_CAPTURE)) {
    return false;
  }
  auto* compiled_regex{kphp::pcre2::compiled_regex::compile(pattern)};
  if (compiled_regex == nullptr) [[unlikely]] {
    return false;
  }
  auto opt_output{split_regex(regex_info, *compiled_regex, limit, (flags & kphp::regex::PREG_SPLIT_NO_EMPTY) != 0,
                              (flags & kphp::regex::PREG_SPLIT_DELIM_CAPTURE) != 0, (flags & kphp::regex::PREG_SPLIT_OFFSET_CAPTURE) != 0)};
  if (!opt_output.has_value()) [[unlikely]] {
    return false;
  }

  return *std::move(opt_output);
}
