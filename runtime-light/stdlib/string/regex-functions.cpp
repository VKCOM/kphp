// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime-light/stdlib/string/regex-functions.h"

#include <algorithm>
#include <array>
#include <cinttypes>
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <limits>
#include <memory>
#include <optional>
#include <string_view>
#include <type_traits>

#include "common/containers/final_action.h"
#include "runtime-common/core/allocator/runtime-allocator.h"
#include "runtime-common/core/runtime-core.h"
#include "runtime-common/core/utils/kphp-assert-core.h"
#include "runtime-common/stdlib/string/mbstring-functions.h"
#include "runtime-light/stdlib/string/regex-include.h"
#include "runtime-light/stdlib/string/regex-state.h"

namespace {

constexpr size_t ERROR_BUFFER_LENGTH = 256;

enum class trailing_unmatch : uint8_t { skip, include };

using regex_pcre2_group_names_t = memory_resource::stl::vector<const char *, memory_resource::unsynchronized_pool_resource>;

struct RegexInfo final {
  std::string_view regex;
  // non-null-terminated regex without delimiters and PCRE modifiers
  //
  // regex       ->  ~pattern~im\0
  // regex_body  ->   pattern
  std::string_view regex_body;
  std::string_view subject;
  std::string_view replacement;

  // PCRE compile options of the regex
  uint32_t compile_options{};
  // number of groups including entire match
  uint32_t capture_count{};
  // compiled regex
  regex_pcre2_code_t regex_code{nullptr};

  // vector of group names
  regex_pcre2_group_names_t group_names;

  int32_t match_count{};
  size_t match_options{PCRE2_NO_UTF_CHECK};

  int64_t replace_count{};
  // contains a string after replacements if replace_count > 0, nullopt otherwise
  std::optional<string> opt_replace_result;

  RegexInfo() = delete;

  RegexInfo(std::string_view regex_, std::string_view subject_, std::string_view replacement_,
            memory_resource::unsynchronized_pool_resource &memory_resource_) noexcept
    : regex(regex_)
    , subject(subject_)
    , replacement(replacement_)
    , group_names(regex_pcre2_group_names_t::allocator_type{memory_resource_}) {}
};

bool valid_preg_replace_mixed(const mixed &param) noexcept {
  if (!param.is_array() && !param.is_string()) [[unlikely]] {
    php_warning("invalid parameter: expected to be string or array");
    return false;
  }
  return true;
}

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
  const vk::final_action finalizer{[&regex_info]() noexcept {
    if (regex_info.regex_code != nullptr) [[likely]] {
      pcre2_pattern_info_8(regex_info.regex_code, PCRE2_INFO_CAPTURECOUNT, std::addressof(regex_info.capture_count));
      ++regex_info.capture_count; // to also count entire match
    } else {
      regex_info.capture_count = 0;
    }
  }};

  auto &regex_state{RegexInstanceState::get()};
  if (!regex_state.compile_context) [[unlikely]] {
    return false;
  }

  // check runtime cache
  if (const auto it{regex_state.regex_pcre2_code_cache.find(regex_info.regex)}; it != regex_state.regex_pcre2_code_cache.end()) {
    regex_info.regex_code = it->second;
    return true;
  }
  // compile pcre2_code
  int32_t error_number{};
  PCRE2_SIZE error_offset{};
  regex_pcre2_code_t regex_code{pcre2_compile_8(reinterpret_cast<PCRE2_SPTR8>(regex_info.regex_body.data()), regex_info.regex_body.size(),
                                                regex_info.compile_options, std::addressof(error_number), std::addressof(error_offset),
                                                regex_state.compile_context.get())};
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

bool collect_group_names(RegexInfo &regex_info) noexcept {
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
  for (auto i = 0; i < name_count; ++i) {
    const auto group_number{static_cast<uint16_t>((entry[0] << 8) | entry[1])};
    PCRE2_SPTR8 group_name{std::next(entry, 2)};
    regex_info.group_names[group_number] = reinterpret_cast<const char *>(group_name);
    std::advance(entry, name_entry_size);
  }

  return true;
}

bool match_regex(RegexInfo &regex_info, size_t offset) noexcept {
  regex_info.match_count = 0;
  const auto &regex_state{RegexInstanceState::get()};
  if (regex_info.regex_code == nullptr || !regex_state.match_context) [[unlikely]] {
    return false;
  }

  int32_t match_count{pcre2_match_8(regex_info.regex_code, reinterpret_cast<PCRE2_SPTR8>(regex_info.subject.data()), regex_info.subject.size(), offset,
                                    regex_info.match_options, regex_state.regex_pcre2_match_data.get(), regex_state.match_context.get())};
  // From https://www.pcre.org/current/doc/html/pcre2_match.html
  // The return from pcre2_match() is one more than the highest numbered capturing pair that has been set
  // (for example, 1 if there are no captures), zero if the vector of offsets is too small, or a negative error code for no match and other errors.
  if (match_count < 0 && match_count != PCRE2_ERROR_NOMATCH) [[unlikely]] {
    std::array<char, ERROR_BUFFER_LENGTH> buffer{};
    pcre2_get_error_message_8(match_count, reinterpret_cast<PCRE2_UCHAR8 *>(buffer.data()), buffer.size());
    php_warning("can't match pcre2 regex due to error: %s", buffer.data());
    return false;
  }
  regex_info.match_count = match_count != PCRE2_ERROR_NOMATCH ? match_count : 0;
  return true;
}

// returns the ending offset of the entire match
PCRE2_SIZE set_matches(const RegexInfo &regex_info, int64_t flags, mixed &matches, trailing_unmatch last_unmatched_policy) noexcept {
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

  // reserve enough space for output
  array<mixed> output{array_size{static_cast<int64_t>(regex_info.group_names.size() + named_groups_count), named_groups_count == 0}};
  for (auto i = 0; i < regex_info.group_names.size(); ++i) {
    // skip unmatched groups at the end unless unmatched_as_null is set
    if (last_unmatched_policy == trailing_unmatch::skip && i > last_matched_group && !unmatched_as_null) [[unlikely]] {
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

    if (regex_info.group_names[i] != nullptr) {
      output.set_value(string{regex_info.group_names[i]}, output_val);
    }
    output.push_back(output_val);
  }

  matches = output;
  return ovector[1];
}

// returns the ending offset of the entire match
// *** importrant ***
// in case of a pattern order all_matches must already contain all groups as empty arrays before the first call to set_all_matches
PCRE2_SIZE set_all_matches(const RegexInfo &regex_info, int64_t flags, mixed &all_matches) noexcept {
  const auto pattern_order{!static_cast<bool>(flags & PREG_SET_ORDER)};

  mixed matches;
  PCRE2_SIZE offset{set_matches(regex_info, flags, matches, pattern_order ? trailing_unmatch::include : trailing_unmatch::skip)};
  if (offset == PCRE2_UNSET) [[unlikely]] {
    return offset;
  }

  if (pattern_order) [[likely]] {
    for (auto &it : matches) {
      all_matches[it.get_key()].push_back(it.get_value());
    }
  } else {
    all_matches.push_back(matches);
  }

  return offset;
}

bool replace_regex(RegexInfo &regex_info, uint64_t limit) noexcept {
  regex_info.replace_count = 0;
  if (regex_info.regex_code == nullptr) [[unlikely]] {
    return false;
  }

  const auto &regex_state{RegexInstanceState::get()};
  auto &runtime_ctx{RuntimeContext::get()};
  if (!regex_state.match_context) [[unlikely]] {
    return false;
  }

  const PCRE2_SIZE buffer_length{std::max({static_cast<string::size_type>(regex_info.subject.size()),
                                           static_cast<string::size_type>(RegexInstanceState::REPLACE_BUFFER_SIZE), runtime_ctx.static_SB.size()})};
  runtime_ctx.static_SB.clean().reserve(buffer_length);
  PCRE2_SIZE output_length{buffer_length};

  // replace all occurences
  if (limit == std::numeric_limits<uint64_t>::max()) [[likely]] {
    regex_info.replace_count =
      pcre2_substitute_8(regex_info.regex_code, reinterpret_cast<PCRE2_SPTR8>(regex_info.subject.data()), regex_info.subject.size(), 0, PCRE2_SUBSTITUTE_GLOBAL,
                         nullptr, regex_state.match_context.get(), reinterpret_cast<PCRE2_SPTR8>(regex_info.replacement.data()), regex_info.replacement.size(),
                         reinterpret_cast<PCRE2_UCHAR8 *>(runtime_ctx.static_SB.buffer()), std::addressof(output_length));

    if (regex_info.replace_count < 0) [[unlikely]] {
      std::array<char, ERROR_BUFFER_LENGTH> buffer{};
      pcre2_get_error_message_8(regex_info.replace_count, reinterpret_cast<PCRE2_UCHAR8 *>(buffer.data()), buffer.size());
      php_warning("pcre2_substitute error %s", buffer.data());
      return false;
    }
  } else { // replace only 'limit' times
    size_t match_offset{};
    size_t substitute_offset{};
    int64_t replacement_diff_acc{};
    PCRE2_SIZE length_after_replace{buffer_length};
    string str_after_replace{regex_info.subject.data(), static_cast<string::size_type>(regex_info.subject.size())};

    for (; regex_info.replace_count < limit; ++regex_info.replace_count) {
      if (!match_regex(regex_info, match_offset)) [[unlikely]] {
        return false;
      }
      if (regex_info.match_count == 0) {
        break;
      }

      const auto *ovector{pcre2_get_ovector_pointer_8(regex_state.regex_pcre2_match_data.get())};
      const auto match_start{ovector[0]};
      const auto match_end{ovector[1]};

      length_after_replace = buffer_length;
      if (auto replace_one{pcre2_substitute_8(regex_info.regex_code, reinterpret_cast<PCRE2_SPTR8>(str_after_replace.c_str()), str_after_replace.size(),
                                              substitute_offset, 0, nullptr, regex_state.match_context.get(),
                                              reinterpret_cast<PCRE2_SPTR8>(regex_info.replacement.data()), regex_info.replacement.size(),
                                              reinterpret_cast<PCRE2_UCHAR8 *>(runtime_ctx.static_SB.buffer()), std::addressof(length_after_replace))};
          replace_one != 1) [[unlikely]] {
        php_warning("pcre2_substitute error %d", replace_one);
        return false;
      }

      match_offset = match_end;
      replacement_diff_acc += regex_info.replacement.size() - (match_end - match_start);
      substitute_offset = match_end + replacement_diff_acc;
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

} // namespace

Optional<int64_t> f$preg_match(const string &pattern, const string &subject, mixed &matches, int64_t flags, int64_t offset) noexcept {
  matches = array<mixed>{};
  RegexInfo regex_info{{pattern.c_str(), pattern.size()}, {subject.c_str(), subject.size()}, {}, RuntimeAllocator::get().memory_resource};

  bool success{valid_regex_flags(flags, PREG_NO_FLAGS, PREG_OFFSET_CAPTURE, PREG_UNMATCHED_AS_NULL)};
  if (!success) [[unlikely]] {
    php_warning("invalid preg_match flags %" PRIi64, flags);
    return false;
  }
  success &= correct_offset(offset, regex_info.subject);
  success &= parse_regex(regex_info);
  success &= compile_regex(regex_info);
  success &= collect_group_names(regex_info);
  success &= match_regex(regex_info, offset);
  if (!success) [[unlikely]] {
    return false;
  }

  set_matches(regex_info, flags, matches, trailing_unmatch::skip);
  return regex_info.match_count > 0 ? 1 : 0;
}

Optional<int64_t> f$preg_match_all(const string &pattern, const string &subject, mixed &matches, int64_t flags, int64_t offset) noexcept {
  matches = array<mixed>{};
  int64_t entire_match_count{};
  RegexInfo regex_info{{pattern.c_str(), pattern.size()}, {subject.c_str(), subject.size()}, {}, RuntimeAllocator::get().memory_resource};

  bool success{valid_regex_flags(flags, PREG_NO_FLAGS, PREG_PATTERN_ORDER, PREG_SET_ORDER, PREG_OFFSET_CAPTURE, PREG_UNMATCHED_AS_NULL)};
  if (!success) [[unlikely]] {
    php_warning("invalid preg_match_all flags %" PRIi64, flags);
    return false;
  }
  success &= correct_offset(offset, regex_info.subject);
  success &= parse_regex(regex_info);
  success &= compile_regex(regex_info);
  success &= collect_group_names(regex_info);

  // pre-init matches in case of pattern order
  if (success && !static_cast<bool>(flags & PREG_SET_ORDER)) [[likely]] {
    const array<mixed> init_val{};
    for (const auto *group_name : regex_info.group_names) {
      if (group_name != nullptr) {
        matches.set_value(string{group_name}, init_val);
      }
      matches.push_back(init_val);
    }
  }

  while (offset <= subject.size() && (success &= match_regex(regex_info, offset))) {
    const auto next_offset{set_all_matches(regex_info, flags, matches)};
    if (regex_info.match_count > 0) {
      ++entire_match_count;
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

Optional<string> f$preg_replace(const string &pattern, const string &replacement, const string &subject, int64_t limit, int64_t &count) noexcept {
  count = 0;
  if (limit < 0 && limit != PREG_REPLACE_NOLIMIT) [[unlikely]] {
    php_warning("invalid limit %" PRIi64 " in preg_replace", limit);
    return {};
  }

  auto &runtime_alloc{RuntimeAllocator::get()};

  string pcre2_replacement{replacement};
  { // we need to replace PHP's back references with PCRE2 ones
    static constexpr std::string_view backreference_pattern = R"(/\\(\d)/)";
    static constexpr std::string_view backreference_replacement = "$$$1";

    RegexInfo regex_info{backreference_pattern, {replacement.c_str(), replacement.size()}, backreference_replacement, runtime_alloc.memory_resource};
    bool success{parse_regex(regex_info)};
    success &= compile_regex(regex_info);
    success &= replace_regex(regex_info, std::numeric_limits<uint64_t>::max());
    if (!success) [[unlikely]] {
      php_warning("can't replace PHP back references with PCRE2 ones");
      return {};
    }
    pcre2_replacement = regex_info.opt_replace_result.has_value() ? *std::move(regex_info.opt_replace_result) : replacement;
  }

  RegexInfo regex_info{{pattern.c_str(), pattern.size()},
                       {subject.c_str(), subject.size()},
                       {pcre2_replacement.c_str(), pcre2_replacement.size()},
                       runtime_alloc.memory_resource};

  bool success{parse_regex(regex_info)};
  success &= compile_regex(regex_info);
  success &= replace_regex(regex_info, limit == PREG_REPLACE_NOLIMIT ? std::numeric_limits<uint64_t>::max() : static_cast<uint64_t>(limit));
  if (!success) [[unlikely]] {
    return {};
  }
  count = regex_info.replace_count;
  return regex_info.opt_replace_result.value_or(subject);
}

Optional<string> f$preg_replace(const mixed &pattern, const string &replacement, const string &subject, int64_t limit, int64_t &count) noexcept {
  count = 0;
  if (!valid_preg_replace_mixed(pattern)) [[unlikely]] {
    return {};
  }

  if (pattern.is_string()) {
    return f$preg_replace(pattern.as_string(), replacement, subject, limit, count);
  }

  string result{subject};
  const auto &pattern_arr{pattern.as_array()};
  for (const auto &it : pattern_arr) {
    int64_t replace_one_count{};
    if (auto replace_result{f$preg_replace(it.get_value().to_string(), replacement, result, limit, replace_one_count)}; replace_result.has_value()) [[likely]] {
      count += replace_one_count;
      result = std::move(replace_result.val());
    } else {
      count = 0;
      return {};
    }
  }

  return result;
}

Optional<string> f$preg_replace(const mixed &pattern, const mixed &replacement, const string &subject, int64_t limit, int64_t &count) noexcept {
  count = 0;
  if (!valid_preg_replace_mixed(pattern) || !valid_preg_replace_mixed(replacement)) [[unlikely]] {
    return {};
  }

  if (replacement.is_string()) {
    return f$preg_replace(pattern, replacement.as_string(), subject, limit, count);
  }
  if (pattern.is_string()) [[unlikely]] {
    php_warning("parameter mismatch: replacement is an array while pattern is string");
    return {};
  }

  string result{subject};
  const auto &pattern_arr{pattern.as_array()};
  const auto &replacement_arr{replacement.as_array()};
  auto replacement_it{replacement_arr.cbegin()};
  for (const auto &pattern_it : pattern_arr) {
    string replacement_str{};
    if (replacement_it != replacement_arr.cend()) {
      replacement_str = replacement_it.get_value().to_string();
      ++replacement_it;
    }

    int64_t replace_one_count{};
    if (auto replace_result{f$preg_replace(pattern_it.get_value().to_string(), replacement_str, result, limit, replace_one_count)}; replace_result.has_value())
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

mixed f$preg_replace(const mixed &pattern, const mixed &replacement, const mixed &subject, int64_t limit, int64_t &count) noexcept {
  count = 0;
  if (!valid_preg_replace_mixed(pattern) || !valid_preg_replace_mixed(replacement) || !valid_preg_replace_mixed(subject)) [[unlikely]] {
    return {};
  }

  if (subject.is_string()) {
    return f$preg_replace(pattern, replacement, subject.as_string(), limit, count);
  }

  const auto &subject_arr{subject.as_array()};
  array<mixed> result{subject_arr.size()};
  for (const auto &it : subject_arr) {
    int64_t replace_one_count{};
    if (auto replace_result{f$preg_replace(pattern, replacement, it.get_value().to_string(), limit, replace_one_count)}; replace_result.has_value())
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
