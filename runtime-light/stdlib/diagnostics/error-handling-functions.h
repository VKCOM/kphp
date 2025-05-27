//  Compiler for PHP (aka KPHP)
//  Copyright (c) 2025 LLC «V Kontakte»
//  Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <array>
#include <cstddef>
#include <format>

#include "runtime-common/core/runtime-core.h"
#include "runtime-light/stdlib/diagnostics/stacktrace.h"
#include "runtime-light/utils/logs.h"

inline array<array<string>> f$debug_backtrace() noexcept {
  static constexpr size_t MAX_STACKTRACE_DEPTH = 64;
  static constexpr size_t LOG_BUFFER_SIZE = 32;

  std::array<void*, MAX_STACKTRACE_DEPTH> raw_trace{};
  size_t num_frames{kphp::diagnostic::backtrace(raw_trace)};
  auto resolved_backtrace{kphp::diagnostic::backtrace_code_address(std::span<void*>(raw_trace.data(), num_frames))};
  if (resolved_backtrace.empty()) [[unlikely]] {
    kphp::log::warning("Cannot resolve virtual addresses to static offsets");
  }

  array<array<string>> backtrace{array_size{static_cast<int64_t>(num_frames), true}};
  const string function_key{"function"};

  for (const auto& address : resolved_backtrace) {
    std::array<char, LOG_BUFFER_SIZE> log_buffer{};
    const auto [_, recorded]{std::format_to_n(log_buffer.data(), log_buffer.size() - 1, "{}", address)};
    array<string> frame_info{array_size{1, false}};
    frame_info.set_value(function_key, string{log_buffer.data(), static_cast<string::size_type>(recorded)});
    backtrace.emplace_back(std::move(frame_info));
  }

  return backtrace;
}
