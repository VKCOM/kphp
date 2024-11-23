// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime-light/stdlib/string/regex-functions.h"

#include <array>
#include <cinttypes>
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <memory>
#include <string_view>
#include <type_traits>

#include "runtime-common/core/allocator/runtime-allocator.h"
#include "runtime-common/core/runtime-core.h"
#include "runtime-common/core/utils/kphp-assert-core.h"
#include "runtime-common/stdlib/string/mbstring-functions.h"
#include "runtime-light/stdlib/string/regex-include.h"
#include "runtime-light/stdlib/string/regex-state.h"

namespace {

constexpr size_t ERROR_BUFFER_LENGTH = 256;

enum class trailing_unmatch_policy_t : uint8_t { skip, include };

struct RegexInfo final {
  std::string_view regex;
  std::string_view subject;
  // non-null-terminated regex without delimiters and PCRE modifiers
  //
  // regex       ->  ~pattern~im\0
  // regex_body  ->   pattern
  std::string_view regex_body;

  // PCRE compile options of the regex
  uint32_t compile_options{};
  // compiled regex
  regex_pcre2_code_t regex_code{nullptr};

  size_t match_options{PCRE2_NO_UTF_CHECK};
  // matched regex info
  int32_t match_count{};
};

template<typename... Args>
requires((std::is_same_v<Args, int64_t> && ...) && sizeof...(Args) > 0) bool valid_regex_flags(int64_t flags, Args... supported_flags) noexcept {
  return (flags & ~(supported_flags | ...)) == PREG_NO_FLAGS;
}

bool correct_offset(int64_t &offset, std::string_view subject) noexcept {
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

bool parse_regex(RegexInfo &regex_info) noexcept {
  if (regex_info.regex.empty()) {
    php_warning("empty regex");
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
      php_warning("wrong regex delimiter %c", start_delim);
      return false;
    }
  }

  uint32_t compile_options{};
  regex_info.regex_body = regex_info.regex;

  // remove start delimiter
  regex_info.regex_body.remove_prefix(1);
  // parse compile options and skip all symbols until the end delimiter
  for (; !regex_info.regex_body.empty() && regex_info.regex_body.back() != end_delim; regex_info.regex_body.remove_suffix(1)) {
    // spaces and newlines are ignored
    if (regex_info.regex_body.back() == ' ' || regex_info.regex_body.back() == '\n') {
      continue;
    }

    switch (regex_info.regex_body.back()) {
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
        php_warning("unsupported regex modifier %c", regex_info.regex_body.back());
        break;
      }
    }
  }

  if (regex_info.regex_body.empty()) {
    php_warning("no ending regex delimiter");
    return false;
  }
  // UTF-8 validation
  if (static_cast<bool>(compile_options & PCRE2_UTF)) {
    if (!mb_UTF8_check(regex_info.regex.data())) [[unlikely]] {
      php_warning("invalid UTF-8 pattern");
      return false;
    }
    if (!mb_UTF8_check(regex_info.subject.data())) [[unlikely]] {
      php_warning("invalid UTF-8 subject");
      return false;
    }
  }

  // remove the end delimiter
  regex_info.regex_body.remove_suffix(1);
  regex_info.compile_options = compile_options;
  return true;
}

bool compile_regex(RegexInfo &regex_info) noexcept {
  // 1. check compile time cache
  // 2. check runtime cache
  // 3. compile

  auto &regex_state{RegexInstanceState::get()};
  // check runtime cache
  if (const auto it{regex_state.regex_pcre2_code_cache.find(regex_info.regex)}; it != regex_state.regex_pcre2_code_cache.end()) {
    regex_info.regex_code = it->second;
    return true;
  }
  // compile pcre2_code
  int32_t error_number{};
  PCRE2_SIZE error_offset{};
  const regex_pcre2_compile_context_t compile_context{pcre2_compile_context_create_8(regex_state.regex_pcre2_general_context.get()),
                                                      pcre2_compile_context_free_8};
  if (!compile_context) [[unlikely]] {
    php_warning("can't create pcre2_compile_context");
    return false;
  }

  regex_pcre2_code_t regex_code{pcre2_compile_8(reinterpret_cast<PCRE2_SPTR8>(regex_info.regex_body.data()), regex_info.regex_body.size(),
                                                regex_info.compile_options, std::addressof(error_number), std::addressof(error_offset), compile_context.get())};
  if (!regex_code) [[unlikely]] {
    std::array<char, ERROR_BUFFER_LENGTH> buffer{};
    pcre2_get_error_message_8(error_number, reinterpret_cast<PCRE2_UCHAR8 *>(buffer.data()), buffer.size());
    php_warning("can't compile pcre2 regex due to error at offset %zu: %s", error_offset, buffer.data());
    return false;
  }

  // add compiled code to runtime cache
  regex_state.regex_pcre2_code_cache.emplace(regex_info.regex, regex_code);

  regex_info.regex_code = regex_code;
  return true;
}

bool match_regex(RegexInfo &regex_info, size_t offset) noexcept {
  if (regex_info.regex_code == nullptr) {
    return false;
  }

  const auto &regex_state{RegexInstanceState::get()};

  const regex_pcre2_match_context_t match_context{pcre2_match_context_create_8(regex_state.regex_pcre2_general_context.get()), pcre2_match_context_free_8};
  if (!match_context) [[unlikely]] {
    php_warning("can't create pcre2_match_context");
    return false;
  }

  int32_t match_count{pcre2_match_8(regex_info.regex_code, reinterpret_cast<PCRE2_SPTR8>(regex_info.subject.data()), regex_info.subject.size(), offset,
                                    regex_info.match_options, regex_state.regex_pcre2_match_data.get(), match_context.get())};
  // From https://www.pcre.org/current/doc/html/pcre2_match.html
  // The return from pcre2_match() is one more than the highest numbered capturing pair that has been set
  // (for example, 1 if there are no captures), zero if the vector of offsets is too small, or a negative error code for no match and other errors.
  if (match_count < 0 && match_count != PCRE2_ERROR_NOMATCH) [[unlikely]] {
    php_warning("can't match pcre2 regex due to error %d", match_count);
    return false;
  }
  regex_info.match_count = match_count != PCRE2_ERROR_NOMATCH ? match_count : 0;
  return true;
}

regex_pcre2_group_names_vector_t get_group_names(const RegexInfo &regex_info) noexcept {
  using allocator_t = regex_pcre2_group_names_vector_t::allocator_type;
  auto &memory_resource{RuntimeAllocator::get().memory_resource};
  regex_pcre2_group_names_vector_t group_names{allocator_t{memory_resource}};

  if (regex_info.regex_code == nullptr) [[unlikely]] {
    return group_names;
  }

  uint32_t capture_count{};
  pcre2_pattern_info_8(regex_info.regex_code, PCRE2_INFO_CAPTURECOUNT, std::addressof(capture_count));
  ++capture_count; // to also count the entire match

  // initialize an array of strings to hold group names
  group_names.resize(capture_count);

  uint32_t name_count{};
  pcre2_pattern_info_8(regex_info.regex_code, PCRE2_INFO_NAMECOUNT, std::addressof(name_count));
  if (name_count == 0) {
    return group_names;
  }

  PCRE2_SPTR8 name_table{};
  uint32_t name_entry_size{};
  pcre2_pattern_info_8(regex_info.regex_code, PCRE2_INFO_NAMETABLE, std::addressof(name_table));
  pcre2_pattern_info_8(regex_info.regex_code, PCRE2_INFO_NAMEENTRYSIZE, std::addressof(name_entry_size));

  PCRE2_SPTR8 entry{name_table};
  for (auto i = 0; i < name_count; ++i) {
    const auto group_number{static_cast<uint16_t>((entry[0] << 8) | entry[1])};
    PCRE2_SPTR8 group_name{std::next(entry, 2)};
    group_names[group_number] = reinterpret_cast<const char *>(group_name);
    std::advance(entry, name_entry_size);
  }

  return group_names;
}

// returns the ending offset of the entire match
PCRE2_SIZE set_matches(const RegexInfo &regex_info, int64_t flags, mixed &matches, trailing_unmatch_policy_t last_unmatched_policy) noexcept {
  if (regex_info.regex_code == nullptr || regex_info.match_count <= 0) [[unlikely]] {
    return PCRE2_UNSET;
  }

  const auto &regex_state{RegexInstanceState::get()};

  const auto offset_capture{static_cast<bool>(flags & PREG_OFFSET_CAPTURE)};
  const auto unmatched_as_null{static_cast<bool>(flags & PREG_UNMATCHED_AS_NULL)};
  // get the ouput vector from the match data
  const auto *ovector{pcre2_get_ovector_pointer_8(regex_state.regex_pcre2_match_data.get())};
  // calculate last matched group
  int64_t last_matched_group{-1};
  for (auto i = 0; i < regex_info.match_count; ++i) {
    if (ovector[static_cast<ptrdiff_t>(2 * i)] != PCRE2_UNSET) {
      last_matched_group = i;
    }
  }
  // retrieve the named groups count
  uint32_t named_groups_count{};
  pcre2_pattern_info_8(regex_info.regex_code, PCRE2_INFO_NAMECOUNT, std::addressof(named_groups_count));
  // get group names
  const auto group_names{get_group_names(regex_info)};

  // reserve enough space for output
  array<mixed> output{array_size{static_cast<int64_t>(group_names.size() + named_groups_count), named_groups_count == 0}};
  for (auto i = 0; i < group_names.size(); ++i) {
    // skip unmatched groups at the end unless unmatched_as_null is set
    if (last_unmatched_policy == trailing_unmatch_policy_t::skip && i > last_matched_group && !unmatched_as_null) [[unlikely]] {
      break;
    }

    const auto match_start{ovector[static_cast<ptrdiff_t>(2 * i)]};
    const auto match_end{ovector[static_cast<ptrdiff_t>(2 * i + 1)]};

    mixed match_val;                  // NULL value
    if (match_start != PCRE2_UNSET) { // handle matched group
      const auto match_size{match_end - match_start};
      match_val = string{std::next(regex_info.subject.data(), match_start), static_cast<string::size_type>(match_size)};
    } else if (!unmatched_as_null) { // handle unmatched group
      match_val = string{};
    }

    mixed output_val;
    if (offset_capture) {
      output_val = array<mixed>::create(std::move(match_val), static_cast<int64_t>(match_start));
    } else {
      output_val = std::move(match_val);
    }

    if (group_names[i] != nullptr) {
      output.set_value(string{group_names[i]}, output_val);
    }
    output.push_back(output_val);
  }

  matches = output;
  return ovector[1];
}

// returns the ending offset of the entire match
PCRE2_SIZE set_all_matches(const RegexInfo &regex_info, int64_t flags, mixed &all_matches) noexcept {
  const auto pattern_order{!static_cast<bool>(flags & PREG_SET_ORDER)};

  mixed matches;
  PCRE2_SIZE offset{set_matches(regex_info, flags, matches, pattern_order ? trailing_unmatch_policy_t::include : trailing_unmatch_policy_t::skip)};
  if (offset == PCRE2_UNSET) [[unlikely]] {
    return offset;
  }

  if (pattern_order) [[likely]] {
    for (auto &it : matches) {
      auto &group{all_matches[it.get_key()]};
      if (f$is_null(group)) [[unlikely]] {
        group = array<mixed>{};
      }

      group.push_back(it.get_value());
    }
  } else {
    all_matches.push_back(matches);
  }

  return offset;
}

} // namespace

Optional<int64_t> f$preg_match(const string &pattern, const string &subject, mixed &matches, int64_t flags, int64_t offset) noexcept {
  matches = array<mixed>{};
  RegexInfo regex_info{.regex = {pattern.c_str(), pattern.size()}, .subject = {subject.c_str(), subject.size()}};

  bool success{valid_regex_flags(flags, PREG_NO_FLAGS, PREG_OFFSET_CAPTURE, PREG_UNMATCHED_AS_NULL)};
  if (!success) [[unlikely]] {
    php_warning("invalid preg_match flags %" PRIi64, flags);
    return false;
  }
  success &= correct_offset(offset, regex_info.subject);
  success &= parse_regex(regex_info);
  success &= compile_regex(regex_info);
  success &= match_regex(regex_info, offset);
  if (!success) [[unlikely]] {
    return false;
  }

  set_matches(regex_info, flags, matches, trailing_unmatch_policy_t::skip);
  return regex_info.match_count > 0 ? 1 : 0;
}

Optional<int64_t> f$preg_match_all(const string &pattern, const string &subject, mixed &matches, int64_t flags, int64_t offset) noexcept {
  int64_t entire_match_count{};
  matches = array<mixed>{};
  RegexInfo regex_info{.regex = {pattern.c_str(), pattern.size()}, .subject = {subject.c_str(), subject.size()}};

  bool success{valid_regex_flags(flags, PREG_NO_FLAGS, PREG_PATTERN_ORDER, PREG_SET_ORDER, PREG_OFFSET_CAPTURE, PREG_UNMATCHED_AS_NULL)};
  if (!success) [[unlikely]] {
    php_warning("invalid preg_match_all flags %" PRIi64, flags);
    return false;
  }
  success &= correct_offset(offset, regex_info.subject);
  success &= parse_regex(regex_info);
  success &= compile_regex(regex_info);
  while (offset <= subject.size() && (success &= match_regex(regex_info, offset))) {
    if (regex_info.match_count > 0) {
      ++entire_match_count;
      const auto next_offset{set_all_matches(regex_info, flags, matches)};
      if (next_offset == PCRE2_UNSET) [[unlikely]] {
        break;
      } else if (next_offset == offset) [[unlikely]] {
        offset = next_offset + 1;
      } else {
        offset = next_offset;
      }
    } else {
      ++offset;
      offset = static_cast<bool>(regex_info.compile_options & PCRE2_UTF) ? skip_utf8_subsequent_bytes(offset, regex_info.subject) : offset;
    }
  }
  if (!success) [[unlikely]] {
    return false;
  }

  return entire_match_count;
}
