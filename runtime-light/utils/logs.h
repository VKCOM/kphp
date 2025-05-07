// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <array>
#include <cstddef>
#include <format>
#include <source_location>
#include <utility>

#include "runtime-light/k2-platform/k2-api.h"

enum class LogLevel : size_t {
  Error = 1,
  Warn = 2,
  Info = 3,
  Debug = 4,
  Trace = 5,
};

namespace kphp::log {

namespace impl {

enum class level : size_t { error = 1, warn, info, debug, trace };

template<typename... Args>
void log(level level, const std::format_string<Args...>& fmt, Args&&... args) noexcept {
  if (std::to_underlying(level) > k2::log_level_enabled()) {
    return;
  }

  static constexpr size_t LOG_BUFFER_SIZE = 512;
  std::array<char, LOG_BUFFER_SIZE> log_buffer{};
  const auto [out, size]{std::format_to_n(log_buffer.data(), log_buffer.size() - 1, fmt, std::forward<Args>(args)...)};
  *out = '\0';
  k2::log(std::to_underlying(level), size, log_buffer.data());
}

} // namespace impl

template<typename... Args>
[[noreturn]] void fatal(const std::format_string<Args...>& fmt, Args&&... args) noexcept {
  impl::log(impl::level::error, fmt, std::forward<Args>(args)...);
  k2::exit(1);
}

inline void assertion(bool condition, const std::source_location& location = std::source_location::current()) noexcept {
  if (condition) [[likely]] {
    return;
  }
  kphp::log::fatal("assertion failed at {}:{}", location.file_name(), location.line());
}

template<typename... Args>
void warning(const std::format_string<Args...>& fmt, Args&&... args) noexcept {
  impl::log(impl::level::warn, fmt, std::forward<Args>(args)...);
}

template<typename... Args>
void info(const std::format_string<Args...>& fmt, Args&&... args) noexcept {
  impl::log(impl::level::info, fmt, std::forward<Args>(args)...);
}

template<typename... Args>
void debug(const std::format_string<Args...>& fmt, Args&&... args) noexcept {
  impl::log(impl::level::debug, fmt, std::forward<Args>(args)...);
}

template<typename... Args>
void trace(const std::format_string<Args...>& fmt, Args&&... args) noexcept {
  impl::log(impl::level::trace, fmt, std::forward<Args>(args)...);
}

} // namespace kphp::log
