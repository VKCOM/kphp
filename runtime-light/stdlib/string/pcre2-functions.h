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

struct error {
  int32_t code{};
};

struct compile_error : error {
  size_t offset{};
};

struct group_name {
  std::string_view name;
  size_t index;
};

class group_name_iterator {
public:
  using iterator_category = std::forward_iterator_tag;
  using value_type = group_name;
  using difference_type = std::ptrdiff_t;
  using pointer = group_name*;
  using reference = group_name;

  group_name_iterator(const PCRE2_UCHAR8* current_entry, uint32_t entry_size) noexcept
      : m_ptr{current_entry},
        m_entry_size{entry_size} {}

  group_name operator*() const noexcept {
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

private:
  const PCRE2_UCHAR8* m_ptr;
  uint32_t m_entry_size;
};

class regex {
public:
  friend class matcher;

  static std::expected<regex, compile_error> compile(std::string_view pattern, uint32_t options = 0, pcre2_compile_context_8* ctx = nullptr) noexcept;

  group_name_iterator names_begin() const noexcept;

  group_name_iterator names_end() const noexcept;

  auto names() const noexcept {
    struct range {
      group_name_iterator b;
      group_name_iterator e;
      group_name_iterator begin() const noexcept {
        return b;
      }
      group_name_iterator end() const noexcept {
        return e;
      }
    };
    return range{.b = names_begin(), .e = names_end()};
  }

  uint32_t capture_count() const noexcept {
    uint32_t count{};
    pcre2_pattern_info_8(m_code.get(), PCRE2_INFO_CAPTURECOUNT, &count);
    return count;
  }

  uint32_t name_count() const noexcept {
    uint32_t count{};
    pcre2_pattern_info_8(m_code.get(), PCRE2_INFO_NAMECOUNT, &count);
    return count;
  }

  bool is_utf() const noexcept {
    uint32_t compile_options{};
    pcre2_pattern_info_8(m_code.get(), PCRE2_INFO_ARGOPTIONS, &compile_options);
    return (compile_options & PCRE2_UTF) != 0;
  }

  /**
   * @param buffer_len Input: capacity of buffer. Output: actual length of result.
   * @return expected<size_t, error>: The number of replacements (should be 1).
   */
  std::expected<size_t, error> substitute_match(std::string_view subject, pcre2_match_data_8& data, std::string_view replacement, char* buffer,
                                                size_t& buffer_len, uint32_t match_options, pcre2_match_context_8* ctx = nullptr) const noexcept;

private:
  explicit regex(pcre2_code_8& code)
      : m_code{std::addressof(code), pcre2_code_free_8} {}
  regex_pcre2_code_t m_code;
};

class match_view {
public:
  match_view(std::string_view subject, const PCRE2_SIZE* ovector, size_t num_groups) noexcept
      : m_subject_data{subject},
        m_ovector_ptr{ovector},
        m_num_groups{num_groups} {}

  int32_t size() const noexcept {
    return m_num_groups;
  }

  struct offset_range {
    size_t start;
    size_t end;
  };

  std::optional<offset_range> get_range(size_t i = 0) const noexcept;

  std::optional<std::string_view> get_group(size_t i) const noexcept {
    if (auto range{get_range(i)}; range.has_value()) {
      return m_subject_data.substr(range->start, range->end - range->start);
    }
    return std::nullopt;
  }

  struct group_content {
    std::string_view text;
    size_t offset;
  };

  std::optional<group_content> get_group_content(size_t i) const noexcept {
    if (auto range{get_range(i)}; range.has_value()) {
      return group_content{
        .text = m_subject_data.substr(range->start, range->end - range->start),
        .offset = range->start
      };
    }
    return std::nullopt;
  }

  size_t match_start() const noexcept {
    return m_ovector_ptr[0];
  }
  size_t match_end() const noexcept {
    return m_ovector_ptr[1];
  }

private:
  std::string_view m_subject_data;
  const PCRE2_SIZE* m_ovector_ptr;
  size_t m_num_groups;
};

class matcher {
public:
  matcher(const regex& re, std::string_view subject, size_t match_from, pcre2_match_context_8* ctx, pcre2_match_data_8& data, uint32_t options = 0) noexcept;

  std::expected<std::optional<match_view>, error> next() noexcept;

  uint32_t get_last_success_options() const noexcept {
    return m_last_success_options;
  }

private:
  const regex& m_re;
  std::string_view m_subject;
  pcre2_match_context_8* m_ctx;
  PCRE2_SIZE m_current_offset{};
  pcre2_match_data_8& m_match_data;
  uint32_t m_base_options;
  uint32_t m_match_options{};
  uint32_t m_last_success_options{};
  bool m_is_utf;
};

struct error_formatter_base {
  static constexpr size_t ERROR_BUFFER_LENGTH{256};

  template<typename ParseContext>
  constexpr auto parse(ParseContext& ctx) const noexcept {
    return ctx.begin();
  }

  struct error_msg_view {
    std::array<unsigned char, ERROR_BUFFER_LENGTH> buffer;
    int32_t length;

    struct result {
      std::string_view text;
      bool truncated;
    };

    result get_result() const& noexcept {
      if (length >= 0) {
        return {.text = {reinterpret_cast<const char*>(buffer.data()), static_cast<size_t>(length)}, .truncated = false};
      }

      if (length == PCRE2_ERROR_NOMEMORY) {
        return {.text = std::string_view{reinterpret_cast<const char*>(buffer.data())}, .truncated = true};
      }

      switch (length) {
      case PCRE2_ERROR_BADDATA:
        return {"unrecognized PCRE2 error code", false};
      default:
        return {"unknown internal PCRE2 error", false};
      }
    }

    result get_result() const&& = delete;
  };

  error_msg_view get_msg_view(int32_t code) const noexcept {
    error_msg_view ev;
    ev.length = pcre2_get_error_message_8(code, ev.buffer.data(), ev.buffer.size());
    return ev;
  }
};

} // namespace kphp::pcre2

namespace std {

template<>
struct formatter<kphp::pcre2::error> : kphp::pcre2::error_formatter_base {
  template<typename FmtContext>
  auto format(const kphp::pcre2::error& err, FmtContext& ctx) const noexcept {
    auto msg_data{get_msg_view(err.code)};
    auto msg{msg_data.get_result()};

    return format_to(ctx.out(), "PCRE2 error {}: {}{}", err.code, msg.text, msg.truncated ? "..." : "");
  }
};

template<>
struct formatter<kphp::pcre2::compile_error> : kphp::pcre2::error_formatter_base {
  template<typename FmtContext>
  auto format(const kphp::pcre2::compile_error& err, FmtContext& ctx) const noexcept {
    auto msg_data{get_msg_view(err.code)};
    auto msg{msg_data.get_result()};

    return format_to(ctx.out(), "PCRE2 compilation error {} at offset {}: {}{}", err.code, err.offset, msg.text, msg.truncated ? "..." : "");
  }
};

} // namespace std
