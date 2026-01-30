// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include <optional>
#include <utility>

#include "common/mixin/not_copyable.h"
#include "runtime-common/core/allocator/script-allocator.h"
#include "runtime-common/core/allocator/script-malloc-interface.h"
#include "runtime-common/core/runtime-core.h"
#include "runtime-common/core/std/containers.h"
#include "runtime-light/stdlib/diagnostics/logs.h"
#include "runtime-light/stdlib/string/pcre2-functions.h"
// correctly include PCRE2 lib
#include "runtime-light/stdlib/string/regex-include.h"

struct RegexInstanceState final : private vk::not_copyable {
  struct compiled_regex {
    // PCRE compile options of the regex
    uint32_t compile_options{};
    // compiled regex
    kphp::pcre2::regex regex_code;
  };

private:
  using hasher_type = decltype([](const string& s) noexcept { return static_cast<size_t>(s.hash()); });

  static constexpr size_t MAX_SUBPATTERNS_COUNT{512};

  kphp::stl::unordered_map<string, compiled_regex, kphp::memory::script_allocator, hasher_type> regex_pcre2_code_cache;

  static void* regex_malloc(PCRE2_SIZE size, [[maybe_unused]] void* memory_data) noexcept {
    auto* mem{kphp::memory::script::alloc(size)};
    if (mem == nullptr) [[unlikely]] {
      kphp::log::warning("regex malloc: can't allocate {} bytes", size);
    }
    return mem;
  }

  static void regex_free(void* mem, [[maybe_unused]] void* memory_data) noexcept {
    if (mem == nullptr) [[unlikely]] {
      return;
    }
    kphp::memory::script::free(mem);
  }

public:
  static constexpr size_t OVECTOR_SIZE{MAX_SUBPATTERNS_COUNT + 1};
  static constexpr size_t REPLACE_BUFFER_SIZE{size_t{16U} * size_t{1024U}};

  kphp::pcre2::general_context general_context;
  kphp::pcre2::compile_context compile_context;
  kphp::pcre2::match_context match_context;
  kphp::pcre2::match_data match_data;

  RegexInstanceState() noexcept
      : general_context(pcre2_general_context_create_8(regex_malloc, regex_free, nullptr), pcre2_general_context_free_8),
        compile_context(pcre2_compile_context_create_8(general_context.get()), pcre2_compile_context_free_8),
        match_context(pcre2_match_context_create_8(general_context.get()), pcre2_match_context_free_8),
        match_data(pcre2_match_data_create_8(OVECTOR_SIZE, general_context.get()), pcre2_match_data_free_8) {
    if (!general_context) [[unlikely]] {
      kphp::log::error("can't create pcre2_general_context");
    }
    if (!compile_context) [[unlikely]] {
      kphp::log::error("can't create pcre2_compile_context");
    }
    if (!match_context) [[unlikely]] {
      kphp::log::error("can't create pcre2_match_context");
    }
    if (!match_data) [[unlikely]] {
      kphp::log::error("can't create match_data");
    }
  }

  std::optional<std::reference_wrapper<const compiled_regex>> get_compiled_regex(const string& regex) const noexcept {
    if (const auto it{regex_pcre2_code_cache.find(regex)}; it != regex_pcre2_code_cache.end()) {
      return it->second;
    }
    return std::nullopt;
  }

  std::optional<std::reference_wrapper<const compiled_regex>> add_compiled_regex(string regex, uint32_t compile_options,
                                                                                 kphp::pcre2::regex regex_code) noexcept {
    return regex_pcre2_code_cache.emplace(std::move(regex), compiled_regex{.compile_options = compile_options, .regex_code = std::move(regex_code)})
        .first->second;
  }

  static RegexInstanceState& get() noexcept;
};
