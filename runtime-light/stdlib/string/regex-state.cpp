// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime-light/stdlib/string/regex-state.h"

#include <memory>
#include <utility>

#include "runtime-common/core/allocator/script-malloc-interface.h"
#include "runtime-common/core/runtime-core.h"
#include "runtime-light/state/instance-state.h"
#include "runtime-light/stdlib/diagnostics/logs.h"
#include "runtime-light/stdlib/string/regex-include.h"

namespace {

void* regex_malloc(PCRE2_SIZE size, [[maybe_unused]] void* memory_data) noexcept {
  auto* mem{kphp::memory::script::alloc(size)};
  if (mem == nullptr) [[unlikely]] {
    kphp::log::warning("regex malloc: can't allocate {} bytes", size);
  }
  return mem;
}

void regex_free(void* mem, [[maybe_unused]] void* memory_data) noexcept {
  if (mem == nullptr) [[unlikely]] {
    return;
  }
  kphp::memory::script::free(mem);
}

} // namespace

RegexInstanceState::RegexInstanceState() noexcept
    : regex_pcre2_general_context(pcre2_general_context_create_8(regex_malloc, regex_free, nullptr), pcre2_general_context_free_8),
      compile_context(pcre2_compile_context_create_8(regex_pcre2_general_context.get()), pcre2_compile_context_free_8),
      match_context(pcre2_match_context_create_8(regex_pcre2_general_context.get()), pcre2_match_context_free_8),
      regex_pcre2_match_data(pcre2_match_data_create_8(OVECTOR_SIZE, regex_pcre2_general_context.get()), pcre2_match_data_free_8) {
  if (!regex_pcre2_general_context) [[unlikely]] {
    kphp::log::error("can't create pcre2_general_context");
  }
  if (!compile_context) [[unlikely]] {
    kphp::log::error("can't create pcre2_compile_context");
  }
  if (!match_context) [[unlikely]] {
    kphp::log::error("can't create pcre2_match_context");
  }
}

const RegexInstanceState::compiled_regex* RegexInstanceState::get_compiled_regex(const string& regex) const noexcept {
  if (const auto it{regex_pcre2_code_cache.find(regex)}; it != regex_pcre2_code_cache.end()) {
    return std::addressof(it->second);
  }
  return nullptr;
}

const RegexInstanceState::compiled_regex* RegexInstanceState::add_compiled_regex(string regex, uint32_t compile_options, pcre2_code_8& regex_code) noexcept {
  return std::addressof(
      regex_pcre2_code_cache.emplace(std::move(regex), compiled_regex{.compile_options = compile_options, .regex_code = regex_code}).first->second);
}

RegexInstanceState& RegexInstanceState::get() noexcept {
  return InstanceState::get().regex_instance_state;
}
