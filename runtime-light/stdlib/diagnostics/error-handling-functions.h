//  Compiler for PHP (aka KPHP)
//  Copyright (c) 2025 LLC «V Kontakte»
//  Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <format>
#include <span>
#include <utility>

#include "runtime-common/core/runtime-core.h"
#include "runtime-light/k2-platform/k2-api.h"
#include "runtime-light/stdlib/diagnostics/backtrace.h"
#include "runtime-light/utils/logs.h"

namespace error_handling_impl_ {
inline constexpr std::string_view FUNCTION_KEY = "function";
inline constexpr std::string_view FILENAME_KEY = "file";
inline constexpr std::string_view LINE_KEY = "line";

inline array<array<string>> format_backtrace_symbols(std::span<void* const> backtrace) noexcept {

  auto resolved_backtrace{kphp::diagnostic::backtrace_symbols(backtrace)};
  if (resolved_backtrace.empty()) {
    return {};
  }

  array<array<string>> backtrace_symbols{array_size{static_cast<int64_t>(backtrace.size()), true}};
  const string function_key{FUNCTION_KEY.data(), FUNCTION_KEY.size()};
  const string filename_key{FILENAME_KEY.data(), FILENAME_KEY.size()};
  const string line_key{LINE_KEY.data(), LINE_KEY.size()};

  for (const k2::SymbolInfo& symbol_info : resolved_backtrace) {
    array<string> frame_info{array_size{3, false}};
    frame_info.set_value(function_key, string{symbol_info.name.get()});
    frame_info.set_value(filename_key, string{symbol_info.filename.get()});
    frame_info.set_value(line_key, string{static_cast<int64_t>(symbol_info.lineno)});

    backtrace_symbols.emplace_back(std::move(frame_info));
  }

  return backtrace_symbols;
}

inline array<array<string>> format_backtrace_addresses(std::span<void* const> backtrace) noexcept {
  static constexpr size_t LOG_BUFFER_SIZE = 32;

  auto resolved_backtrace{kphp::diagnostic::backtrace_addresses(backtrace)};
  if (resolved_backtrace.empty()) {
    return {};
  }

  array<array<string>> backtrace_addresses{array_size{static_cast<int64_t>(backtrace.size()), true}};
  const string function_key{FUNCTION_KEY.data(), FUNCTION_KEY.size()};

  for (const auto& address : resolved_backtrace) {
    std::array<char, LOG_BUFFER_SIZE> log_buffer{};
    const auto [out, recorded]{std::format_to_n(log_buffer.data(), log_buffer.size() - 1, "{}", address)};
    *out = '\0';
    array<string> frame_info{array_size{1, false}};
    frame_info.set_value(function_key, string{log_buffer.data(), static_cast<string::size_type>(recorded)});

    backtrace_addresses.emplace_back(std::move(frame_info));
  }

  return backtrace_addresses;
}
} // namespace error_handling_impl_

inline array<array<string>> f$debug_backtrace() noexcept {
  static constexpr size_t MAX_STACKTRACE_DEPTH = 64;

  std::array<void*, MAX_STACKTRACE_DEPTH> backtrace{};
  size_t num_frames{kphp::diagnostic::backtrace(backtrace)};
  std::span<void*> backtrace_view{backtrace.data(), num_frames};
  auto backtrace_symbols{error_handling_impl_::format_backtrace_symbols(backtrace_view)};
  if (!backtrace_symbols.empty()) {
    return backtrace_symbols;
  }
  auto backtrace_code_addresses{error_handling_impl_::format_backtrace_addresses(backtrace_view)};
  if (!backtrace_code_addresses.empty()) {
    return backtrace_code_addresses;
  }
  kphp::log::warning("cannot resolve virtual addresses to debug information");
  return {};
}
