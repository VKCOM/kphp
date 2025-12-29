// Compiler for PHP (aka KPHP)
// Copyright (c) 2025 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime-light/stdlib/string/pcre2-functions.h"

namespace kphp::pcre2 {

namespace {

int64_t skip_utf8_subsequent_bytes(size_t offset, const std::string_view subject) noexcept {
  // all multibyte utf8 runes consist of subsequent bytes,
  // these subsequent bytes start with 10 bit pattern
  // 0xc0 selects the two most significant bits, then we compare it to 0x80 (0b10000000)
  while (offset < subject.size() && ((static_cast<unsigned char>(subject[offset])) & 0xc0) == 0x80) {
    offset++;
  }
  return offset;
}

} // namespace

std::expected<regex, compile_error> regex::compile(std::string_view pattern, uint32_t options, pcre2_compile_context_8* ctx) noexcept {
  int32_t errorcode{};
  PCRE2_SIZE erroroffset{};

  pcre2_code_8* re{pcre2_compile_8(reinterpret_cast<PCRE2_SPTR>(pattern.data()), pattern.length(), options, &errorcode, &erroroffset, ctx)};

  if (!re) {
    return std::unexpected{compile_error{{.code = errorcode}, erroroffset}};
  }
  return regex{*re};
}

regex::group_name_range regex::names() const noexcept {
  uint32_t count{};
  uint32_t entry_size{};
  PCRE2_SPTR8 table{};

  pcre2_pattern_info_8(m_code.get(), PCRE2_INFO_NAMECOUNT, std::addressof(count));

  if (count == 0) {
    return {.b = group_name_iterator{nullptr, 0}, .e = group_name_iterator{nullptr, 0}};
  }

  pcre2_pattern_info_8(m_code.get(), PCRE2_INFO_NAMEENTRYSIZE, std::addressof(entry_size));
  pcre2_pattern_info_8(m_code.get(), PCRE2_INFO_NAMETABLE, std::addressof(table));

  return {.b = group_name_iterator{table, entry_size}, .e = group_name_iterator{std::next(table, static_cast<size_t>(count) * entry_size), entry_size}};
}

std::expected<size_t, error> regex::substitute_match(std::string_view subject, pcre2_match_data_8& data, std::string_view replacement, char* buffer,
                                                     size_t& buffer_len, uint32_t match_options, pcre2_match_context_8* ctx) const noexcept {
  uint32_t substitute_options{PCRE2_SUBSTITUTE_UNKNOWN_UNSET | PCRE2_SUBSTITUTE_UNSET_EMPTY | PCRE2_SUBSTITUTE_MATCHED | PCRE2_SUBSTITUTE_OVERFLOW_LENGTH |
                              PCRE2_SUBSTITUTE_REPLACEMENT_ONLY | match_options};

  auto rc{pcre2_substitute_8(m_code.get(), reinterpret_cast<PCRE2_SPTR8>(subject.data()), subject.length(), 0, substitute_options, std::addressof(data), ctx,
                             reinterpret_cast<PCRE2_SPTR8>(replacement.data()), replacement.length(), reinterpret_cast<PCRE2_UCHAR8*>(buffer),
                             std::addressof(buffer_len))};

  if (rc < 0) {
    return std::unexpected<error>{{.code = rc}};
  }

  return static_cast<size_t>(rc);
}

std::optional<match_view::offset_range> match_view::get_range(size_t i) const noexcept {
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
  return offset_range{.start = start, .end = end};
}

matcher::matcher(const regex& re, std::string_view subject, size_t match_from, pcre2_match_context_8* ctx, pcre2_match_data_8& data, uint32_t options) noexcept
    : m_re{re},
      m_subject{subject},
      m_ctx{ctx},
      m_current_offset{match_from},
      m_match_data{data},
      m_base_options{options},
      m_is_utf{re.is_utf()} {}

std::expected<std::optional<match_view>, error> matcher::next() noexcept {
  while (m_current_offset <= m_subject.length()) {
    uint32_t current_attempt_options{m_base_options | m_match_options};

    auto ret_code{pcre2_match_8(m_re.m_code.get(), reinterpret_cast<PCRE2_SPTR8>(m_subject.data()), m_subject.length(), m_current_offset,
                                current_attempt_options, std::addressof(m_match_data), m_ctx)};

    if (ret_code == PCRE2_ERROR_NOMATCH) {
      if (m_match_options != 0) {
        // If the anchored non-empty match failed, advance 1 unit and try again
        m_match_options = 0;
        m_current_offset++;
        if (m_is_utf) {
          m_current_offset = skip_utf8_subsequent_bytes(m_current_offset, m_subject);
        }
        continue;
      }
      return std::nullopt;
    }

    // From https://www.pcre.org/current/doc/html/pcre2_match.html
    // The return from pcre2_match() is one more than the highest numbered capturing pair that has been set
    // (for example, 1 if there are no captures), zero if the vector of offsets is too small, or a negative error code for no match and other errors.
    if (ret_code < 0) [[unlikely]] {
      return std::unexpected{error{.code = ret_code}};
    }

    m_last_success_options = current_attempt_options;

    size_t matched_groups_count{};
    if (ret_code == 0) {
      matched_groups_count = pcre2_get_ovector_count_8(std::addressof(m_match_data));
    } else {
      matched_groups_count = static_cast<size_t>(ret_code);
    }

    const PCRE2_SIZE* ovector{pcre2_get_ovector_pointer_8(std::addressof(m_match_data))};

    size_t start{ovector[0]};
    size_t end{ovector[1]};

    if (start == end) {
      // Found an empty match; set flags to try finding a non-empty match at same position
      m_match_options = PCRE2_NOTEMPTY_ATSTART | PCRE2_ANCHORED;
    } else {
      m_match_options = 0;
    }
    m_current_offset = end;

    return match_view{m_subject, ovector, matched_groups_count};
  }

  return std::nullopt;
}

} // namespace kphp::pcre2
