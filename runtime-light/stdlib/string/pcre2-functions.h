// Compiler for PHP (aka KPHP)
// Copyright (c) 2025 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cstddef>
#include <cstdint>
#include <expected>
#include <format>
#include <iterator>
#include <optional>
#include <string_view>

#include "runtime-light/stdlib/diagnostics/logs.h"
#include "runtime-light/stdlib/string/regex-include.h"

namespace kphp::pcre2 {

namespace details {

inline int64_t skip_utf8_subsequent_bytes(size_t offset, const std::string_view subject) noexcept {
  // all multibyte utf8 runes consist of subsequent bytes,
  // these subsequent bytes start with 10 bit pattern
  // 0xc0 selects the two most significant bits, then we compare it to 0x80 (0b10000000)
  while (offset < subject.size() && ((static_cast<unsigned char>(subject[offset])) & 0xc0) == 0x80) {
    offset++;
  }
  return offset;
}

} // namespace details

using regex_general_context_t = std::unique_ptr<pcre2_general_context_8, decltype(std::addressof(pcre2_general_context_free_8))>;
using regex_compile_context_t = std::unique_ptr<pcre2_compile_context_8, decltype(std::addressof(pcre2_compile_context_free_8))>;
using regex_match_context_t = std::unique_ptr<pcre2_match_context_8, decltype(std::addressof(pcre2_match_context_free_8))>;
using regex_match_data_t = std::unique_ptr<pcre2_match_data_8, decltype(std::addressof(pcre2_match_data_free_8))>;
using regex_code_t = std::unique_ptr<pcre2_code_8, decltype(std::addressof(pcre2_code_free_8))>;

struct error {
  int32_t code{};
};

struct compile_error : kphp::pcre2::error {
  size_t offset{};
};

struct group_name {
  std::string_view name;
  size_t index{};
};

class group_name_iterator {
  const PCRE2_UCHAR8* m_ptr{nullptr};
  size_t m_entry_size{};

public:
  using iterator_category = std::forward_iterator_tag;
  using value_type = kphp::pcre2::group_name;
  using difference_type = std::ptrdiff_t;
  using pointer = kphp::pcre2::group_name*;
  using reference = kphp::pcre2::group_name;

  group_name_iterator() = delete;
  group_name_iterator(const PCRE2_UCHAR8* current_entry, size_t entry_size) noexcept
      : m_ptr{current_entry},
        m_entry_size{entry_size} {}

  kphp::pcre2::group_name operator*() const noexcept {
    const auto index{static_cast<size_t>(m_ptr[0] << 8 | m_ptr[1])};
    const auto* name_ptr{reinterpret_cast<const char*>(std::next(m_ptr, 2))};
    return {.name = std::string_view{name_ptr}, .index = index};
  }

  group_name_iterator& operator++() noexcept {
    std::advance(m_ptr, m_entry_size);
    return *this;
  }

  group_name_iterator operator++(int) noexcept {
    group_name_iterator tmp{*this};
    ++*this;
    return tmp;
  }

  bool operator==(const group_name_iterator& other) const noexcept {
    return m_ptr == other.m_ptr;
  }
};

class regex {
  kphp::pcre2::regex_code_t m_code;

public:
  friend class match_view;
  friend class matcher;

  static std::expected<regex, kphp::pcre2::compile_error> compile(std::string_view pattern, const kphp::pcre2::regex_compile_context_t& ctx,
                                                                  uint32_t options = 0) noexcept {
    int32_t errorcode{};
    PCRE2_SIZE erroroffset{};

    kphp::pcre2::regex_code_t re{pcre2_compile_8(reinterpret_cast<PCRE2_SPTR>(pattern.data()), pattern.length(), options, std::addressof(errorcode),
                                                 std::addressof(erroroffset), ctx.get()),
                                 pcre2_code_free_8};

    if (!re) {
      return std::unexpected{kphp::pcre2::compile_error{{.code = errorcode}, erroroffset}};
    }
    return kphp::pcre2::regex{std::move(re)};
  }

  struct group_name_range {
    kphp::pcre2::group_name_iterator b;
    kphp::pcre2::group_name_iterator e;

    kphp::pcre2::group_name_iterator begin() const noexcept {
      return b;
    }
    kphp::pcre2::group_name_iterator end() const noexcept {
      return e;
    }

    bool empty() const noexcept {
      return b == e;
    }
  };

  group_name_range names() const noexcept {
    uint32_t count{};
    uint32_t entry_size{};
    PCRE2_SPTR8 table{};

    kphp::log::assertion(pcre2_pattern_info_8(m_code.get(), PCRE2_INFO_NAMECOUNT, std::addressof(count)) == 0);

    if (count == 0) {
      return {.b = group_name_iterator{nullptr, 0}, .e = group_name_iterator{nullptr, 0}};
    }

    kphp::log::assertion(pcre2_pattern_info_8(m_code.get(), PCRE2_INFO_NAMEENTRYSIZE, std::addressof(entry_size)) == 0);
    kphp::log::assertion(pcre2_pattern_info_8(m_code.get(), PCRE2_INFO_NAMETABLE, std::addressof(table)) == 0);

    return {.b = group_name_iterator{table, entry_size}, .e = group_name_iterator{std::next(table, static_cast<size_t>(count) * entry_size), entry_size}};
  }

  uint32_t capture_count() const noexcept {
    uint32_t count{};
    kphp::log::assertion(pcre2_pattern_info_8(m_code.get(), PCRE2_INFO_CAPTURECOUNT, std::addressof(count)) == 0);
    return count;
  }

  uint32_t name_count() const noexcept {
    uint32_t count{};
    kphp::log::assertion(pcre2_pattern_info_8(m_code.get(), PCRE2_INFO_NAMECOUNT, std::addressof(count)) == 0);
    return count;
  }

  bool is_utf() const noexcept {
    uint32_t compile_options{};
    kphp::log::assertion(pcre2_pattern_info_8(m_code.get(), PCRE2_INFO_ARGOPTIONS, std::addressof(compile_options)) == 0);
    return (compile_options & PCRE2_UTF) != 0;
  }

private:
  explicit regex(kphp::pcre2::regex_code_t&& code) noexcept
      : m_code{std::move(code)} {}
};

class match_view {
  const kphp::pcre2::regex& m_re;
  std::string_view m_subject;
  const kphp::pcre2::regex_match_data_t& m_match_data;
  uint32_t m_match_options{};
  size_t m_num_groups{};

public:
  match_view(const regex& re, std::string_view subject, const kphp::pcre2::regex_match_data_t& match_data, uint32_t match_options, size_t num_groups) noexcept
      : m_re{re},
        m_subject{subject},
        m_match_data{match_data},
        m_match_options{match_options},
        m_num_groups{num_groups} {}

  int32_t size() const noexcept {
    return m_num_groups;
  }

  struct offset_range {
    size_t start{};
    size_t end{};
  };

  std::optional<std::string_view> get_group(size_t i) const noexcept {
    if (auto range{get_range(i)}; range.has_value()) {
      return m_subject.substr(range->start, range->end - range->start);
    }
    return std::nullopt;
  }

  struct group_content {
    std::string_view text;
    size_t offset{};
  };

  std::optional<group_content> get_group_content(size_t i) const noexcept {
    if (auto range{get_range(i)}; range.has_value()) {
      return group_content{.text = m_subject.substr(range->start, range->end - range->start), .offset = range->start};
    }
    return std::nullopt;
  }

  size_t match_start() const noexcept {
    return pcre2_get_ovector_pointer_8(m_match_data.get())[0];
  }
  size_t match_end() const noexcept {
    return pcre2_get_ovector_pointer_8(m_match_data.get())[1];
  }

  /**
   * @param buffer_len Input: capacity of buffer. Output: actual length of result.
   * @return expected<size_t, error>: The number of replacements (should be 1).
   */
  std::expected<size_t, kphp::pcre2::error> substitute(std::string_view replacement, char* buffer, size_t& buffer_len,
                                                       const kphp::pcre2::regex_match_context_t& ctx) const noexcept {
    uint32_t substitute_options{PCRE2_SUBSTITUTE_UNKNOWN_UNSET | PCRE2_SUBSTITUTE_UNSET_EMPTY | PCRE2_SUBSTITUTE_MATCHED | PCRE2_SUBSTITUTE_OVERFLOW_LENGTH |
                                PCRE2_SUBSTITUTE_REPLACEMENT_ONLY | m_match_options};

    auto ret_code{pcre2_substitute_8(m_re.m_code.get(), reinterpret_cast<PCRE2_SPTR8>(m_subject.data()), m_subject.length(), 0, substitute_options,
                                     m_match_data.get(), ctx.get(), reinterpret_cast<PCRE2_SPTR8>(replacement.data()), replacement.length(),
                                     reinterpret_cast<PCRE2_UCHAR8*>(buffer), std::addressof(buffer_len))};

    if (ret_code < 0) {
      return std::unexpected<kphp::pcre2::error>{{.code = ret_code}};
    }

    return static_cast<size_t>(ret_code);
  }

private:
  std::optional<offset_range> get_range(size_t i = 0) const noexcept {
    if (i >= m_num_groups) {
      return std::nullopt;
    }

    const auto* ovector_ptr{pcre2_get_ovector_pointer_8(m_match_data.get())};
    // ovector is an array of offset pairs
    PCRE2_SIZE start{ovector_ptr[2 * i]};
    PCRE2_SIZE end{ovector_ptr[(2 * i) + 1]};

    if (start == PCRE2_UNSET) {
      return std::nullopt;
    }
    return offset_range{.start = start, .end = end};
  }
};

class matcher {
  const kphp::pcre2::regex& m_re;
  std::string_view m_subject;
  const kphp::pcre2::regex_match_context_t& m_ctx;
  PCRE2_SIZE m_current_offset{};
  const kphp::pcre2::regex_match_data_t& m_match_data;
  uint32_t m_base_options{};
  uint32_t m_match_options{};
  bool m_is_utf{false};

public:
  matcher(const kphp::pcre2::regex& re, std::string_view subject, size_t match_from, const kphp::pcre2::regex_match_context_t& ctx,
          const kphp::pcre2::regex_match_data_t& data, uint32_t options = 0) noexcept
      : m_re{re},
        m_subject{subject},
        m_ctx{ctx},
        m_current_offset{match_from},
        m_match_data{data},
        m_base_options{options},
        m_is_utf{re.is_utf()} {}

  std::expected<std::optional<kphp::pcre2::match_view>, kphp::pcre2::error> next() noexcept {
    while (m_current_offset <= m_subject.length()) {
      uint32_t current_attempt_options{m_base_options | m_match_options};

      auto ret_code{pcre2_match_8(m_re.m_code.get(), reinterpret_cast<PCRE2_SPTR8>(m_subject.data()), m_subject.length(), m_current_offset,
                                  current_attempt_options, m_match_data.get(), m_ctx.get())};

      if (ret_code == PCRE2_ERROR_NOMATCH) {
        if (m_match_options != 0) {
          // If the anchored non-empty match failed, advance 1 unit and try again
          m_match_options = 0;
          m_current_offset++;
          if (m_is_utf) {
            m_current_offset = kphp::pcre2::details::skip_utf8_subsequent_bytes(m_current_offset, m_subject);
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

      size_t matched_groups_count{};
      if (ret_code == 0) {
        matched_groups_count = pcre2_get_ovector_count_8(m_match_data.get());
      } else {
        matched_groups_count = static_cast<size_t>(ret_code);
      }

      const PCRE2_SIZE* ovector{pcre2_get_ovector_pointer_8(m_match_data.get())};

      size_t start{ovector[0]};
      size_t end{ovector[1]};

      if (start == end) {
        // Found an empty match; set flags to try finding a non-empty match at same position
        m_match_options = PCRE2_NOTEMPTY_ATSTART | PCRE2_ANCHORED;
      } else {
        m_match_options = 0;
      }
      m_current_offset = end;

      return kphp::pcre2::match_view{m_re, m_subject, m_match_data, current_attempt_options, matched_groups_count};
    }

    return std::nullopt;
  }
};

} // namespace kphp::pcre2

namespace std {

template<>
struct formatter<kphp::pcre2::error> {
  static constexpr size_t ERROR_BUFFER_LENGTH{256};

  template<typename ParseContext>
  constexpr auto parse(ParseContext& ctx) const noexcept {
    return ctx.begin();
  }

  template<typename FmtContext>
  auto format(kphp::pcre2::error error, FmtContext& ctx) const noexcept {
    std::array<char, ERROR_BUFFER_LENGTH> buffer{};
    auto ret_code{pcre2_get_error_message_8(error.code, reinterpret_cast<PCRE2_UCHAR8*>(buffer.data()), buffer.size())};
    if (ret_code < 0) [[unlikely]] {
      switch (ret_code) {
      case PCRE2_ERROR_BADDATA:
        return format_to(ctx.out(), "unknown error ({})", error.code);
      case PCRE2_ERROR_NOMEMORY:
        return format_to(ctx.out(), "[truncated] {}", buffer.data());
      default:
        kphp::log::error("unsupported regex error code: {}", ret_code);
      }
    }
    return format_to(ctx.out(), "{}", buffer.data());
  }
};

} // namespace std
