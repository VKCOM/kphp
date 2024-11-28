// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cstddef>
#include <cstdint>
#include <string_view>

#include "common/mixin/not_copyable.h"
#include "runtime-common/core/memory-resource/resource_allocator.h"
#include "runtime-common/core/memory-resource/unsynchronized_pool_resource.h"
#include "runtime-common/core/runtime-core.h"
#include "runtime-light/stdlib/string/regex-include.h"
#include "runtime-light/utils/concepts.h"

struct RegexInstanceState final : private vk::not_copyable {
  template<hashable Key, typename Value>
  using unordered_map = memory_resource::stl::unordered_map<Key, Value, memory_resource::unsynchronized_pool_resource>;

  static constexpr size_t MAX_SUBPATTERNS_COUNT = 512;
  // match data size should be a multiple of 3 since it holds ovector triples (see pcre2 docs)
  static constexpr size_t MATCH_DATA_SIZE = 3 * MAX_SUBPATTERNS_COUNT;
  static constexpr auto REPLACE_BUFFER_SIZE = static_cast<size_t>(16U * 1024U);

  mixed default_matches;
  int64_t default_preg_replace_count;
  const regex_pcre2_general_context_t regex_pcre2_general_context;
  const regex_pcre2_compile_context_t compile_context;
  const regex_pcre2_match_context_t match_context;
  regex_pcre2_match_data_t regex_pcre2_match_data;
  unordered_map<std::string_view, regex_pcre2_code_t> regex_pcre2_code_cache;

  explicit RegexInstanceState(memory_resource::unsynchronized_pool_resource &memory_resource) noexcept;

  static RegexInstanceState &get() noexcept;
};
