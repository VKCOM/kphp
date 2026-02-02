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
#include <span>
#include <string_view>

#include "runtime-light/stdlib/diagnostics/logs.h"
// correctly include PCRE2 lib
#include "runtime-light/stdlib/string/regex-include.h"

namespace kphp::pcre2 {

namespace details {

namespace offset_pair {

inline constexpr size_t START{0};
inline constexpr size_t END{1};
inline constexpr size_t SIZE{2};

} // namespace offset_pair

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

using general_context = std::unique_ptr<pcre2_general_context_8, decltype(std::addressof(pcre2_general_context_free_8))>;
using compile_context = std::unique_ptr<pcre2_compile_context_8, decltype(std::addressof(pcre2_compile_context_free_8))>;
using match_context = std::unique_ptr<pcre2_match_context_8, decltype(std::addressof(pcre2_match_context_free_8))>;
using match_data = std::unique_ptr<pcre2_match_data_8, decltype(std::addressof(pcre2_match_data_free_8))>;
using code = std::unique_ptr<pcre2_code_8, decltype(std::addressof(pcre2_code_free_8))>;

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

class regex {
  kphp::pcre2::code m_code;

  class group_name_iterator {
    const PCRE2_UCHAR8* m_ptr{nullptr};
    const size_t m_entry_size{};

  public:
    using iterator_category = std::forward_iterator_tag;
    using value_type = kphp::pcre2::group_name;
    using difference_type = std::ptrdiff_t;
    using pointer = kphp::pcre2::group_name*;
    using reference = kphp::pcre2::group_name;

    group_name_iterator() = delete;
    group_name_iterator(const PCRE2_UCHAR8* current_entry, size_t entry_size) noexcept
        : m_ptr{current_entry},
          m_entry_size{entry_size} {
      kphp::log::assertion(current_entry != nullptr);
    }

    kphp::pcre2::group_name operator*() const noexcept {
      static constexpr size_t UPPER = 0;
      static constexpr size_t LOWER = 1;

      const auto index{static_cast<size_t>(m_ptr[UPPER] << 8 | m_ptr[LOWER])};
      const auto* name_ptr{reinterpret_cast<const char*>(std::next(m_ptr, 2 * sizeof(PCRE2_UCHAR8)))};
      return {.name = std::string_view{name_ptr}, .index = index};
    }

    group_name_iterator& operator++() noexcept {
      std::advance(m_ptr, m_entry_size);
      return *this;
    }

    group_name_iterator operator++(int) noexcept { // NOLINT
      group_name_iterator tmp{*this};
      ++*this;
      return tmp;
    }

    bool operator==(const group_name_iterator& other) const noexcept {
      return m_ptr == other.m_ptr;
    }
  };

public:
  friend class match_view;
  friend class matcher;

  static std::expected<regex, kphp::pcre2::compile_error> compile(std::string_view pattern, kphp::pcre2::compile_context& ctx, uint32_t options = 0) noexcept {
    int32_t errorcode{};
    PCRE2_SIZE erroroffset{};

    kphp::pcre2::code re{pcre2_compile_8(reinterpret_cast<PCRE2_SPTR>(pattern.data()), pattern.length(), options, std::addressof(errorcode),
                                         std::addressof(erroroffset), ctx.get()),
                         pcre2_code_free_8};

    if (!re) {
      return std::unexpected{kphp::pcre2::compile_error{{.code = errorcode}, erroroffset}};
    }
    return kphp::pcre2::regex{std::move(re)};
  }

  struct group_name_range {
    group_name_iterator b;
    group_name_iterator e;

    group_name_iterator begin() const noexcept {
      return b;
    }
    group_name_iterator end() const noexcept {
      return e;
    }

    bool empty() const noexcept {
      return b == e;
    }
  };

  group_name_range group_names() const noexcept {
    uint32_t count{};
    uint32_t entry_size{};
    PCRE2_SPTR8 table{};

    kphp::log::assertion(pcre2_pattern_info_8(m_code.get(), PCRE2_INFO_NAMECOUNT, std::addressof(count)) == 0);

    if (count == 0) {
      return {.b = group_name_iterator{nullptr, 0}, .e = group_name_iterator{nullptr, 0}};
    }

    kphp::log::assertion(pcre2_pattern_info_8(m_code.get(), PCRE2_INFO_NAMEENTRYSIZE, std::addressof(entry_size)) == 0 &&
                         pcre2_pattern_info_8(m_code.get(), PCRE2_INFO_NAMETABLE, std::addressof(table)) == 0);

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
  explicit regex(kphp::pcre2::code&& code) noexcept
      : m_code{std::move(code)} {}
};

class match_view {
  const kphp::pcre2::regex& m_re;
  std::string_view m_subject;
  kphp::pcre2::match_data& m_match_data;
  uint32_t m_match_options{};
  size_t m_num_groups{};

public:
  match_view(const regex& re, std::string_view subject, kphp::pcre2::match_data& match_data, uint32_t match_options, size_t num_groups) noexcept
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
    return pcre2_get_ovector_pointer_8(m_match_data.get())[kphp::pcre2::details::offset_pair::START];
  }
  size_t match_end() const noexcept {
    return pcre2_get_ovector_pointer_8(m_match_data.get())[kphp::pcre2::details::offset_pair::END];
  }

  std::expected<size_t, std::pair<size_t, kphp::pcre2::error>> substitute(std::string_view replacement, std::span<char> buffer,
                                                                          kphp::pcre2::match_context& ctx) const noexcept {
    kphp::log::assertion(buffer.data() != nullptr);

    uint32_t substitute_options{PCRE2_SUBSTITUTE_UNKNOWN_UNSET | PCRE2_SUBSTITUTE_UNSET_EMPTY | PCRE2_SUBSTITUTE_MATCHED | PCRE2_SUBSTITUTE_OVERFLOW_LENGTH |
                                PCRE2_SUBSTITUTE_REPLACEMENT_ONLY | m_match_options};

    auto buffer_len{buffer.size()};
    auto ret_code{pcre2_substitute_8(m_re.m_code.get(), reinterpret_cast<PCRE2_SPTR8>(m_subject.data()), m_subject.length(), 0, substitute_options,
                                     m_match_data.get(), ctx.get(), reinterpret_cast<PCRE2_SPTR8>(replacement.data()), replacement.length(),
                                     reinterpret_cast<PCRE2_UCHAR8*>(buffer.data()), std::addressof(buffer_len))};

    if (ret_code < 0) {
      return std::unexpected<std::pair<size_t, kphp::pcre2::error>>{{buffer_len, {.code = ret_code}}};
    }

    return buffer_len;
  }

private:
  std::optional<offset_range> get_range(size_t i) const noexcept {
    if (i >= m_num_groups) {
      return std::nullopt;
    }

    const auto* ovector_ptr{pcre2_get_ovector_pointer_8(m_match_data.get())};
    // ovector is an array of offset pairs
    PCRE2_SIZE start{ovector_ptr[(kphp::pcre2::details::offset_pair::SIZE * i) + kphp::pcre2::details::offset_pair::START]};
    PCRE2_SIZE end{ovector_ptr[(kphp::pcre2::details::offset_pair::SIZE * i) + kphp::pcre2::details::offset_pair::END]};

    if (start == PCRE2_UNSET) {
      return std::nullopt;
    }
    return offset_range{.start = start, .end = end};
  }
};

class matcher {
  const kphp::pcre2::regex& m_re;
  std::string_view m_subject;
  kphp::pcre2::match_context& m_ctx;
  PCRE2_SIZE m_current_offset{};
  kphp::pcre2::match_data& m_match_data;
  uint32_t m_user_options{};
  uint32_t m_match_options{};
  bool m_is_utf{false};

public:
  matcher(const kphp::pcre2::regex& re, std::string_view subject, size_t match_from, kphp::pcre2::match_context& ctx, kphp::pcre2::match_data& data,
          uint32_t options = 0) noexcept
      : m_re{re},
        m_subject{subject},
        m_ctx{ctx},
        m_current_offset{match_from},
        m_match_data{data},
        m_user_options{options},
        m_is_utf{re.is_utf()} {}

  std::expected<std::optional<kphp::pcre2::match_view>, kphp::pcre2::error> next() noexcept {
    while (m_current_offset <= m_subject.length()) {
      uint32_t current_attempt_options{m_user_options | m_match_options};

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

      size_t start{ovector[kphp::pcre2::details::offset_pair::START]};
      size_t end{ovector[kphp::pcre2::details::offset_pair::END]};

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

template<>
struct std::formatter<kphp::pcre2::error> {
  template<typename ParseContext>
  constexpr auto parse(ParseContext& ctx) const noexcept {
    return ctx.begin();
  }

  template<typename FmtContext>
  auto format(kphp::pcre2::error error, FmtContext& ctx) const noexcept {
    static constexpr size_t ERROR_BUFFER_LENGTH{256};

    std::array<char, ERROR_BUFFER_LENGTH> buffer; // NOLINT
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
