// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string_view>

#include "runtime-common/core/allocator/script-allocator.h"
#include "runtime-common/core/runtime-core.h"
#include "runtime-common/core/std/containers.h"
#include "runtime-common/stdlib/string/mbstring-functions.h"
#include "runtime-light/stdlib/string/regex-include.h"

namespace kphp::pcre2 {

using group_names_t = kphp::stl::vector<const char*, kphp::memory::script_allocator>;

class match_view {
public:
  match_view(std::string_view subject, PCRE2_SIZE* ovector, int32_t ret_code) noexcept
      : m_subject_data{subject},
        m_ovector_ptr{ovector},
        m_num_groups{ret_code} {}

  match_view(const match_view& other) noexcept = default;
  match_view(match_view&& other) noexcept = default;
  match_view& operator=(const match_view& other) noexcept = default;
  match_view& operator=(match_view&& other) noexcept = default;

  int32_t size() const noexcept {
    return m_num_groups;
  }

  std::optional<std::string_view> get_group(size_t i) const noexcept;

private:
  std::string_view m_subject_data;
  const PCRE2_SIZE* m_ovector_ptr;
  int32_t m_num_groups;
};

class compiled_regex {
public:
  static const compiled_regex* compile(const string& regex) noexcept;

  bool is_utf() const noexcept {
    return compile_options & PCRE2_UTF;
  }

  bool validate(std::string_view subject) const noexcept {
    // UTF-8 validation
    return !is_utf() || mb_UTF8_check(subject.data());
  }

  group_names_t collect_group_names() const noexcept;
  std::optional<match_view> match(std::string_view subject, size_t offset, uint32_t match_options) const noexcept;
  uint32_t named_groups_count() const noexcept;
  std::optional<string> replace(const string& subject, uint32_t replace_options, std::string_view replacement, uint32_t match_options, uint64_t limit,
                                int64_t& replace_count) const noexcept;

private:
  compiled_regex(uint32_t compile_options, pcre2_code_8& regex_code) noexcept
      : compile_options{compile_options},
        regex_code{regex_code} {}

  uint32_t groups_count() const noexcept;

  // PCRE compile options of the regex
  uint32_t compile_options{};
  // compiled regex
  pcre2_code_8& regex_code;
};

} // namespace kphp::pcre2
