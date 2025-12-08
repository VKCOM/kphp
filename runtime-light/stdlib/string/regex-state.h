// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include <optional>

#include "common/mixin/not_copyable.h"
#include "runtime-common/core/allocator/script-allocator.h"
#include "runtime-common/core/runtime-core.h"
#include "runtime-common/core/std/containers.h"
#include "runtime-light/stdlib/string/regex-include.h"

struct RegexInstanceState final : private vk::not_copyable {
private:
  using hasher_type = decltype([](const string& s) noexcept { return static_cast<size_t>(s.hash()); });

  static constexpr size_t MAX_SUBPATTERNS_COUNT{512};

  struct compiled_regex {
    // PCRE compile options of the regex
    uint32_t compile_options{};
    // compiled regex
    regex_pcre2_code_t regex_code;
  };

  kphp::stl::unordered_map<string, compiled_regex, kphp::memory::script_allocator, hasher_type> regex_pcre2_code_cache;

public:
  static constexpr size_t OVECTOR_SIZE{MAX_SUBPATTERNS_COUNT + 1};
  static constexpr size_t REPLACE_BUFFER_SIZE{16U * 1024U};

  const regex_pcre2_general_context_t regex_pcre2_general_context;
  const regex_pcre2_compile_context_t compile_context;
  const regex_pcre2_match_context_t match_context;
  regex_pcre2_match_data_t regex_pcre2_match_data;

  RegexInstanceState() noexcept;

  std::optional<std::reference_wrapper<const compiled_regex>> get_compiled_regex(const string& regex) const noexcept;

  std::optional<std::reference_wrapper<const compiled_regex>> add_compiled_regex(string regex, uint32_t compile_options, regex_pcre2_code_t regex_code) noexcept;

  static RegexInstanceState& get() noexcept;
};
