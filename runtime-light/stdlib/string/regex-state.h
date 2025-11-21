// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cstddef>
#include <string_view>

#include "common/mixin/not_copyable.h"
#include "runtime-common/core/allocator/script-allocator.h"
#include "runtime-common/core/runtime-core.h"
#include "runtime-common/core/std/containers.h"
#include "runtime-light/stdlib/string/regex-include.h"

struct RegexInstanceState final : private vk::not_copyable {
private:
  template<typename Value>
  using string_unordered_map =
      kphp::stl::unordered_map<string, Value, kphp::memory::script_allocator, decltype([](const string& s) noexcept { return static_cast<size_t>(s.hash()); })>;

  static constexpr size_t MAX_SUBPATTERNS_COUNT = 512;

  struct compiled_regex_cache_entry {
    // PCRE compile options of the regex
    uint32_t compile_options{};
    // compiled regex
    regex_pcre2_code_t regex_code{nullptr};
  };

public:
  static constexpr auto REPLACE_BUFFER_SIZE = static_cast<size_t>(16U * 1024U);

  const regex_pcre2_general_context_t regex_pcre2_general_context;
  const regex_pcre2_compile_context_t compile_context;
  const regex_pcre2_match_context_t match_context;
  regex_pcre2_match_data_t regex_pcre2_match_data;
  string_unordered_map<compiled_regex_cache_entry> regex_pcre2_code_cache;

  RegexInstanceState() noexcept;

  void add_compiled_code(std::string_view regex, uint32_t compile_options, regex_pcre2_code_t regex_code) noexcept {
    regex_pcre2_code_cache.emplace(string{regex.data(), static_cast<string::size_type>(regex.size())},
                                   compiled_regex_cache_entry{.compile_options = compile_options, .regex_code = regex_code});
  }

  static RegexInstanceState& get() noexcept;
};
