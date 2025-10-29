// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cstddef>
#include <string_view>

#include "common/mixin/not_copyable.h"
#include "runtime-common/core/allocator/script-allocator.h"
#include "runtime-common/core/std/containers.h"
#include "runtime-light/stdlib/string/regex-include.h"
#include "runtime-light/utils/concepts.h"

struct pcre2_cache_entry {
  // PCRE compile options of the regex
  uint32_t compile_options{};
  // compiled regex
  regex_pcre2_code_t regex_code{nullptr};
};

struct RegexInstanceState final : private vk::not_copyable {
  template<hashable Key, typename Value>
  using unordered_map = kphp::stl::unordered_map<Key, Value, kphp::memory::script_allocator>;

  static constexpr size_t MAX_SUBPATTERNS_COUNT = 512;
  static constexpr auto REPLACE_BUFFER_SIZE = static_cast<size_t>(16U * 1024U);

  const regex_pcre2_general_context_t regex_pcre2_general_context;
  const regex_pcre2_compile_context_t compile_context;
  const regex_pcre2_match_context_t match_context;
  regex_pcre2_match_data_t regex_pcre2_match_data;
  unordered_map<std::string_view, pcre2_cache_entry> regex_pcre2_code_cache;

  RegexInstanceState() noexcept;

  static RegexInstanceState& get() noexcept;
};
