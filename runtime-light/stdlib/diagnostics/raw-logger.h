// Compiler for PHP (aka KPHP)
// Copyright (c) 2025 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <array>
#include <cstddef>
#include <format>
#include <optional>
#include <span>
#include <string_view>
#include <utility>

#include "runtime-light/k2-platform/k2-api.h"
#include "runtime-light/stdlib/diagnostics/detail/logs.h"

namespace kphp::log {

struct raw_logger final {
  raw_logger() noexcept = delete;

  template<typename... Args>
  static void log(kphp::log::level level, std::optional<std::span<void* const>> trace, std::format_string<impl::wrapped_arg_t<Args>...> fmt, Args&&... args) noexcept {
    if (std::to_underlying(level) > k2::log_level_enabled()) {
      return;
    }

    static constexpr size_t LOG_BUFFER_SIZE = 512UZ;
    std::array<char, LOG_BUFFER_SIZE> log_buffer; // NOLINT
    size_t message_size{impl::format_log_message(log_buffer, fmt, std::forward<Args>(args)...)};
    auto message{std::string_view{log_buffer.data(), static_cast<std::string_view::size_type>(message_size)}};

    if (trace.has_value()) {
      std::array<k2::LogTaggedEntry, 1> tagged_entries; // NOLINT
      static constexpr std::string_view BACKTRACE_KEY = "trace";

      static constexpr size_t BACKTRACE_BUFFER_SIZE = 1024UZ * 4UZ;
      std::array<char, BACKTRACE_BUFFER_SIZE> backtrace_buffer; // NOLINT
      size_t backtrace_size{impl::resolve_log_trace(backtrace_buffer, *trace)};

      tagged_entries[0] =
          k2::LogTaggedEntry{.key = BACKTRACE_KEY.data(), .value = backtrace_buffer.data(), .key_len = BACKTRACE_KEY.size(), .value_len = backtrace_size};
      k2::log(std::to_underlying(level), message, tagged_entries);
      return;
    }
    k2::log(std::to_underlying(level), message, std::nullopt);
  }
};
} // namespace kphp::log
