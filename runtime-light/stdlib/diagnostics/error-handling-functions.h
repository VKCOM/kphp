//  Compiler for PHP (aka KPHP)
//  Copyright (c) 2025 LLC «V Kontakte»
//  Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <array>
#include <cstddef>
#include <format>

#include "runtime-common/core/runtime-core.h"
#include "runtime-light/stdlib/diagnostics/stacktrace.h"

inline array<array<string>> f$debug_backtrace() {
  static constexpr uint32_t MAX_STACKTRACE_DEPTH = 64;

  std::array<void*, MAX_STACKTRACE_DEPTH> raw_trace{};
  std::size_t num_frames{kphp::diagnostic::get_async_stacktrace(raw_trace)};

  array<array<string>> backtrace{array_size{static_cast<int64_t>(num_frames), true}};
  const string function_key{"function"};

  for (std::size_t frame = 0; frame < num_frames; ++frame) {
    std::array<char, MAX_STACKTRACE_DEPTH> trace_record_buffer{};
    const auto [_, recorded]{std::format_to_n(trace_record_buffer.data(), trace_record_buffer.size() - 1, "{}", raw_trace[frame])};
    array<string> frame_info{array_size{1, false}};
    frame_info.set_value(function_key, string{trace_record_buffer.data(), static_cast<string::size_type>(recorded)});
    backtrace.emplace_back(std::move(frame_info));
  }

  return backtrace;
}
