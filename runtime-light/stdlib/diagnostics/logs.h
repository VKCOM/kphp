// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <array>
#include <cstddef>
#include <format>
#include <optional>
#include <source_location>
#include <span>

#include "runtime-light/k2-platform/k2-api.h"
#include "runtime-light/stdlib/diagnostics/backtrace.h"
#include "runtime-light/stdlib/diagnostics/logger.h"

namespace kphp::log {
// The backtrace algorithm relies on the fact that assertion does not call backtrace.
// If assertion is modified, the backtrace algorithm should be updated accordingly
inline void assertion(bool condition, const std::source_location& location = std::source_location::current()) noexcept {
  if (!condition) [[unlikely]] {
    logger::format_log(Level::error, std::nullopt, "assertion failed at {}:{}", location.file_name(), location.line());
    k2::exit(1);
  }
}

template<typename... Args>
[[noreturn]] void error(std::format_string<impl::wrapped_arg_t<Args>...> fmt, Args&&... args) noexcept {
  std::array<void*, kphp::diagnostic::DEFAULT_BACKTRACE_MAX_SIZE> backtrace{};
  const size_t num_frames{kphp::diagnostic::backtrace(backtrace)};
  const std::span<void* const> backtrace_view{backtrace.data(), num_frames};
  logger::format_log(Level::error, backtrace_view, fmt, std::forward<Args>(args)...);
  k2::exit(1);
}

template<typename... Args>
void warning(std::format_string<impl::wrapped_arg_t<Args>...> fmt, Args&&... args) noexcept {
  std::array<void*, kphp::diagnostic::DEFAULT_BACKTRACE_MAX_SIZE> backtrace{};
  const size_t num_frames{kphp::diagnostic::backtrace(backtrace)};
  const std::span<void* const> backtrace_view{backtrace.data(), num_frames};
  logger::format_log(Level::warn, backtrace_view, fmt, std::forward<Args>(args)...);
}

template<typename... Args>
void info(std::format_string<impl::wrapped_arg_t<Args>...> fmt, Args&&... args) noexcept {
  logger::format_log(Level::info, std::nullopt, fmt, std::forward<Args>(args)...);
}

template<typename... Args>
void debug(std::format_string<impl::wrapped_arg_t<Args>...> fmt, Args&&... args) noexcept {
  logger::format_log(Level::debug, std::nullopt, fmt, std::forward<Args>(args)...);
}

template<typename... Args>
void trace(std::format_string<impl::wrapped_arg_t<Args>...> fmt, Args&&... args) noexcept {
  logger::format_log(Level::trace, std::nullopt, fmt, std::forward<Args>(args)...);
}

} // namespace kphp::log

template<typename T>
struct std::formatter<kphp::log::impl::floating_wrapper<T>> {
  constexpr auto parse(std::format_parse_context& ctx) const noexcept {
    return ctx.begin();
  }

  template<typename FormatContext>
  auto format(const kphp::log::impl::floating_wrapper<T>& wrapper, FormatContext& ctx) const noexcept {
    return std::format_to(ctx.out(), "{:.4f}", wrapper.value);
  }
};
