// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime-light/stdlib/string/regex-state.h"

#include "runtime-common/core/memory-resource/unsynchronized_pool_resource.h"
#include "runtime-common/core/utils/kphp-assert-core.h"
#include "runtime-light/k2-platform/k2-api.h"
#include "runtime-light/state/instance-state.h"
#include "runtime-light/stdlib/string/regex-include.h"

namespace regex_state_impl_ {

// TODO: use RuntimeAllocator instead
void *regex_malloc(PCRE2_SIZE size, [[maybe_unused]] void *memory_data) noexcept {
  auto *mem{k2::alloc(size)};
  if (mem == nullptr) [[unlikely]] {
    php_warning("regex malloc: can't allocate %zu bytes", size);
  }
  return mem;
}

void regex_free(void *mem, [[maybe_unused]] void *memory_data) noexcept {
  if (mem == nullptr) [[unlikely]] {
    return;
  }
  k2::free(mem);
}

} // namespace regex_state_impl_

RegexInstanceState::RegexInstanceState(memory_resource::unsynchronized_pool_resource &memory_resource) noexcept
  : default_preg_replace_count()
  , regex_pcre2_general_context(pcre2_general_context_create_8(regex_state_impl_::regex_malloc, regex_state_impl_::regex_free, nullptr),
                                pcre2_general_context_free_8)
  , regex_pcre2_match_data(pcre2_match_data_create_8(3 * MAX_SUBPATTERNS_COUNT, regex_pcre2_general_context.get()), pcre2_match_data_free_8)
  , regex_pcre2_code_cache(decltype(regex_pcre2_code_cache)::allocator_type{memory_resource}) {
  if (!regex_pcre2_general_context) [[unlikely]] {
    php_error("can't create pcre2_general_context");
  }
}

RegexInstanceState &RegexInstanceState::get() noexcept {
  return InstanceState::get().regex_instance_state;
}
