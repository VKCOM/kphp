// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cstdint>
#include <string_view>

#include "common/mixin/not_copyable.h"
#include "runtime-common/core/memory-resource/resource_allocator.h"
#include "runtime-common/core/memory-resource/unsynchronized_pool_resource.h"
#include "runtime-common/core/runtime-core.h"
#include "runtime-light/stdlib/string/regex-include.h"
#include "runtime-light/utils/concepts.h"

namespace regex_state_impl_ {

void *regex_malloc(PCRE2_SIZE size, void *memory_data) noexcept;

void regex_free(void *mem, void *memory_data) noexcept;

} // namespace regex_state_impl_

using regex_pcre2_group_names_vector_t = memory_resource::stl::vector<const char *, memory_resource::unsynchronized_pool_resource>;

struct RegexInstanceState final : private vk::not_copyable {
  template<hashable Key, typename Value>
  using unordered_map = memory_resource::stl::unordered_map<Key, Value, memory_resource::unsynchronized_pool_resource>;

  mixed default_matches;
  int64_t default_preg_replace_count;
  regex_pcre2_general_context_t regex_pcre2_general_context;
  unordered_map<std::string_view, regex_pcre2_code_t> regex_pcre2_code_cache;

  explicit RegexInstanceState(memory_resource::unsynchronized_pool_resource &memory_resource) noexcept;

  static RegexInstanceState &get() noexcept;
};
