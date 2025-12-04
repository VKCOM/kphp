// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime-light/stdlib/string/pcre2-functions.h"

#include <cstddef>
#include <cstdint>
#include <iterator>
#include <memory>
#include <optional>
#include <string_view>

#include "runtime-common/core/runtime-core.h"
#include "runtime-common/stdlib/string/mbstring-functions.h"
#include "runtime-light/stdlib/diagnostics/logs.h"
#include "runtime-light/stdlib/string/regex-state.h"

namespace kphp::pcre2 {

namespace {

constexpr size_t ERROR_BUFFER_LENGTH{256};

}

std::optional<std::string_view> match_view::get_group(size_t i) const noexcept {
  if (i >= m_num_groups) {
    return std::nullopt;
  }

  kphp::log::assertion(m_ovector_ptr);
  // ovector is an array of offset pairs
  PCRE2_SIZE start{m_ovector_ptr[2 * i]};
  PCRE2_SIZE end{m_ovector_ptr[2 * i + 1]};

  if (start == PCRE2_UNSET) {
    return std::nullopt;
  }

  return m_subject_data.substr(start, end - start);
}

const compiled_regex* compiled_regex::compile(const string& regex) noexcept {
  auto& regex_state{RegexInstanceState::get()};
  if (!regex_state.compile_context) [[unlikely]] {
    return nullptr;
  }

  // check runtime cache
  if (auto* compiled_regex{regex_state.get_compiled_regex(regex)}; compiled_regex != nullptr) {
    return compiled_regex;
  }
  if (regex.empty()) {
    kphp::log::warning("empty regex");
    return nullptr;
  }

  char end_delim{};
  switch (const char start_delim{regex[0]}; start_delim) {
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
    return nullptr;
  }
  }

  uint32_t compile_options{};
  // non-null-terminated regex without delimiters and PCRE modifiers
  //
  // regex       ->  ~pattern~im\0
  // regex_body  ->   pattern
  std::string_view regex_body = {regex.c_str(), regex.size()};

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
    kphp::log::warning("no ending regex delimiter: {}", regex.c_str());
    return nullptr;
  }
  // UTF-8 validation
  if (static_cast<bool>(compile_options & PCRE2_UTF) && !mb_UTF8_check(regex.c_str())) [[unlikely]] {
    kphp::log::warning("invalid UTF-8 pattern: {}", regex.c_str());
    return nullptr;
  }

  // remove the end delimiter
  regex_body.remove_suffix(1);

  // compile pcre2_code
  int32_t error_number{};
  PCRE2_SIZE error_offset{};
  regex_pcre2_code_t regex_code{pcre2_compile_8(reinterpret_cast<PCRE2_SPTR8>(regex_body.data()), regex_body.size(), compile_options,
                                                std::addressof(error_number), std::addressof(error_offset), regex_state.compile_context.get())};
  if (!regex_code) [[unlikely]] {
    std::array<char, ERROR_BUFFER_LENGTH> buffer{};
    pcre2_get_error_message_8(error_number, reinterpret_cast<PCRE2_UCHAR8*>(buffer.data()), buffer.size());
    kphp::log::warning("can't compile pcre2 regex due to error at offset {}: {}", error_offset, buffer.data());
    return nullptr;
  }

  return regex_state.add_compiled_regex(regex, {compile_options, *regex_code});
}

group_names_t compiled_regex::collect_group_names() const noexcept {
  // vector of group names
  group_names_t group_names;

  // initialize an array of strings to hold group names
  group_names.resize(groups_count());

  uint32_t name_count{};
  pcre2_pattern_info_8(std::addressof(regex_code), PCRE2_INFO_NAMECOUNT, std::addressof(name_count));
  if (name_count == 0) {
    return group_names;
  }

  PCRE2_SPTR8 name_table{};
  uint32_t name_entry_size{};
  pcre2_pattern_info_8(std::addressof(regex_code), PCRE2_INFO_NAMETABLE, std::addressof(name_table));
  pcre2_pattern_info_8(std::addressof(regex_code), PCRE2_INFO_NAMEENTRYSIZE, std::addressof(name_entry_size));

  PCRE2_SPTR8 entry{name_table};
  for (auto i{0}; i < name_count; ++i) {
    const auto group_number{static_cast<uint16_t>((entry[0] << 8) | entry[1])};
    PCRE2_SPTR8 group_name{std::next(entry, 2)};
    group_names[group_number] = reinterpret_cast<const char*>(group_name);
    std::advance(entry, name_entry_size);
  }

  return group_names;
}

std::optional<match_view> compiled_regex::match(std::string_view subject, size_t offset, uint32_t match_options) const noexcept {
  const auto& regex_state{RegexInstanceState::get()};
  if (!regex_state.match_context) [[unlikely]] {
    return std::nullopt;
  }

  auto* match_data = regex_state.regex_pcre2_match_data.get();

  int32_t match_count{pcre2_match_8(std::addressof(regex_code), reinterpret_cast<PCRE2_SPTR8>(subject.data()), subject.size(), offset, match_options,
                                    match_data, regex_state.match_context.get())};
  // From https://www.pcre.org/current/doc/html/pcre2_match.html
  // The return from pcre2_match() is one more than the highest numbered capturing pair that has been set
  // (for example, 1 if there are no captures), zero if the vector of offsets is too small, or a negative error code for no match and other errors.
  if (match_count < 0 && match_count != PCRE2_ERROR_NOMATCH) [[unlikely]] {
    std::array<char, ERROR_BUFFER_LENGTH> buffer{};
    pcre2_get_error_message_8(match_count, reinterpret_cast<PCRE2_UCHAR8*>(buffer.data()), buffer.size());
    kphp::log::warning("can't match pcre2 regex due to error: {}", buffer.data());
    return std::nullopt;
  }
  // zero if the vector of offsets is too small
  return match_view{subject, pcre2_get_ovector_pointer_8(match_data), match_count != PCRE2_ERROR_NOMATCH ? match_count : 0};
}

uint32_t compiled_regex::named_groups_count() const noexcept {
  // retrieve the named groups count
  uint32_t name_count{};
  pcre2_pattern_info_8(std::addressof(regex_code), PCRE2_INFO_NAMECOUNT, std::addressof(name_count));
  return name_count;
}

std::optional<string> compiled_regex::replace(const string& subject, uint32_t replace_options, std::string_view replacement, uint32_t match_options,
                                              uint64_t limit, int64_t& replace_count) const noexcept {
  replace_count = 0;

  const auto& regex_state{RegexInstanceState::get()};
  auto& runtime_ctx{RuntimeContext::get()};
  if (!regex_state.match_context) [[unlikely]] {
    return std::nullopt;
  }

  if (!validate({subject.c_str(), subject.size()})) [[unlikely]] {
    return std::nullopt;
  }

  const PCRE2_SIZE buffer_length{std::max(
      {static_cast<string::size_type>(subject.size()), static_cast<string::size_type>(RegexInstanceState::REPLACE_BUFFER_SIZE), runtime_ctx.static_SB.size()})};
  runtime_ctx.static_SB.clean().reserve(buffer_length);
  PCRE2_SIZE output_length{buffer_length};

  // replace all occurences
  if (limit == std::numeric_limits<uint64_t>::max()) [[likely]] {
    replace_count = pcre2_substitute_8(std::addressof(regex_code), reinterpret_cast<PCRE2_SPTR8>(subject.c_str()), subject.size(), 0,
                                       replace_options | PCRE2_SUBSTITUTE_GLOBAL, nullptr, regex_state.match_context.get(),
                                       reinterpret_cast<PCRE2_SPTR8>(replacement.data()), replacement.size(),
                                       reinterpret_cast<PCRE2_UCHAR8*>(runtime_ctx.static_SB.buffer()), std::addressof(output_length));

    if (replace_count < 0) [[unlikely]] {
      std::array<char, ERROR_BUFFER_LENGTH> buffer{};
      pcre2_get_error_message_8(replace_count, reinterpret_cast<PCRE2_UCHAR8*>(buffer.data()), buffer.size());
      kphp::log::warning("pcre2_substitute error {}", buffer.data());
      return std::nullopt;
    }
  } else { // replace only 'limit' times
    size_t match_offset{};
    size_t substitute_offset{};
    int64_t replacement_diff_acc{};
    PCRE2_SIZE length_after_replace{buffer_length};
    string str_after_replace{subject};

    for (; replace_count < limit; ++replace_count) {
      auto match_view_opt{match({subject.c_str(), subject.size()}, match_offset, match_options)};
      if (!match_view_opt.has_value()) [[unlikely]] {
        return std::nullopt;
      }
      auto& match_view{*match_view_opt};
      if (match_view.size() == 0) {
        break;
      }

      const auto entire_pattern_match_opt{match_view.get_group({})};
      if (!entire_pattern_match_opt.has_value()) [[unlikely]] {
        return std::nullopt;
      }
      auto entire_pattern_match{*entire_pattern_match_opt};

      length_after_replace = buffer_length;
      if (auto replace_one_ret_code{pcre2_substitute_8(std::addressof(regex_code), reinterpret_cast<PCRE2_SPTR8>(str_after_replace.c_str()),
                                                       str_after_replace.size(), substitute_offset, replace_options, nullptr, regex_state.match_context.get(),
                                                       reinterpret_cast<PCRE2_SPTR8>(replacement.data()), replacement.size(),
                                                       reinterpret_cast<PCRE2_UCHAR8*>(runtime_ctx.static_SB.buffer()), std::addressof(length_after_replace))};
          replace_one_ret_code != 1) [[unlikely]] {
        kphp::log::warning("pcre2_substitute error {}", replace_one_ret_code);
        return std::nullopt;
      }

      match_offset = entire_pattern_match.data() - subject.c_str() + entire_pattern_match.size();
      replacement_diff_acc += replacement.size() - entire_pattern_match.size();
      substitute_offset = match_offset + replacement_diff_acc;
      str_after_replace = {runtime_ctx.static_SB.buffer(), static_cast<string::size_type>(length_after_replace)};
    }

    output_length = length_after_replace;
  }

  if (replace_count > 0) {
    runtime_ctx.static_SB.set_pos(output_length);
    return runtime_ctx.static_SB.str();
  }

  return subject;
}

uint32_t compiled_regex::groups_count() const noexcept {
  // number of groups including entire match
  uint32_t capture_count{};
  pcre2_pattern_info_8(std::addressof(regex_code), PCRE2_INFO_CAPTURECOUNT, std::addressof(capture_count));
  return capture_count + 1; // to also count entire match
}

} // namespace kphp::pcre2
