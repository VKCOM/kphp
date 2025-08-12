// Compiler for PHP (aka KPHP)
// Copyright (c) 2025 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <array>
#include <cstddef>
#include <format>
#include <optional>
#include <string_view>
#include <utility>

#include "runtime-light/k2-platform/k2-api.h"
#include "runtime-light/stdlib/diagnostics/detail/logs.h"

namespace kphp::log {

struct raw_logger {
  raw_logger() noexcept = delete;

  template<typename... Args>
  static void log(level level, std::format_string<impl::wrapped_arg_t<Args>...> fmt, Args&&... args) noexcept {
    if (std::to_underlying(level) > k2::log_level_enabled()) {
      return;
    }

    static constexpr size_t LOG_BUFFER_SIZE = 512UZ;
    std::array<char, LOG_BUFFER_SIZE> log_buffer; // NOLINT
    auto [out, size]{std::format_to_n<decltype(log_buffer.data()), impl::wrapped_arg_t<Args>...>(log_buffer.data(), log_buffer.size() - 1, fmt,
                                                                                                 impl::wrap_log_argument(std::forward<Args>(args))...)};
    *out = '\0';
    auto message{std::string_view{log_buffer.data(), static_cast<std::string_view::size_type>(size)}};
    k2::log(std::to_underlying(level), message, std::nullopt);
  }
};
} // namespace kphp::log
