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
#include "runtime-light/stdlib/diagnostics/contextual-logger.h"
#include "runtime-light/stdlib/diagnostics/error-handling-state.h"
#include "runtime-light/stdlib/diagnostics/raw-logger.h"

namespace kphp::log {

namespace impl {

template<typename... Args>
void select_logger_and_log(level level, std::optional<std::span<void* const>> trace, std::format_string<impl::wrapped_arg_t<Args>...> fmt,
                           Args&&... args) noexcept {
  if (auto logger{contextual_logger::try_get()}; logger.has_value()) [[likely]] {
    (*logger).get().log(level, trace, fmt, std::forward<Args>(args)...);
  } else {
    raw_logger::log(level, trace, fmt, std::forward<Args>(args)...);
  }
}

} // namespace impl

// The backtrace algorithm relies on the fact that assertion does not call backtrace.
// If assertion is modified, the backtrace algorithm should be updated accordingly
inline void assertion(bool condition, const std::source_location& location = std::source_location::current()) noexcept {
  if (!condition) [[unlikely]] {
    impl::select_logger_and_log(level::error, std::nullopt, "assertion failed at {}:{}", location.file_name(), location.line());
    k2::exit(1);
  }
}

template<typename... Args>
[[noreturn]] void error(std::format_string<impl::wrapped_arg_t<Args>...> fmt, Args&&... args) noexcept {
  if (const auto& error_handling_state_opt{ErrorHandlingState::try_get()};
      !error_handling_state_opt.has_value() || (*error_handling_state_opt).get().log_level_enabled(E_ERROR)) {
    if (std::to_underlying(level::error) <= k2::log_level_enabled()) {
      std::array<void*, kphp::diagnostic::DEFAULT_BACKTRACE_MAX_SIZE> backtrace{};
      const size_t num_frames{kphp::diagnostic::backtrace(backtrace)};
      const std::span<void* const> backtrace_view{backtrace.data(), num_frames};
      impl::select_logger_and_log(level::error, backtrace_view, fmt, std::forward<Args>(args)...);
    }
  }
  k2::exit(1);
}

template<typename... Args>
void warning(std::format_string<impl::wrapped_arg_t<Args>...> fmt, Args&&... args) noexcept {
  if (const auto& error_handling_state_opt{ErrorHandlingState::try_get()};
      !error_handling_state_opt.has_value() || (*error_handling_state_opt).get().log_level_enabled(E_WARNING)) {
    if (std::to_underlying(level::warn) <= k2::log_level_enabled()) {
      std::array<void*, kphp::diagnostic::DEFAULT_BACKTRACE_MAX_SIZE> backtrace{};
      const size_t num_frames{kphp::diagnostic::backtrace(backtrace)};
      const std::span<void* const> backtrace_view{backtrace.data(), num_frames};
      impl::select_logger_and_log(level::warn, backtrace_view, fmt, std::forward<Args>(args)...);
    }
  }
}

template<typename... Args>
void info(std::format_string<impl::wrapped_arg_t<Args>...> fmt, Args&&... args) noexcept {
  if (const auto& error_handling_state_opt{ErrorHandlingState::try_get()};
      !error_handling_state_opt.has_value() || (*error_handling_state_opt).get().log_level_enabled(E_NOTICE)) {
    if (std::to_underlying(level::info) <= k2::log_level_enabled()) {
      impl::select_logger_and_log(level::info, std::nullopt, fmt, std::forward<Args>(args)...);
    }
  }
}

template<typename... Args>
void debug(std::format_string<impl::wrapped_arg_t<Args>...> fmt, Args&&... args) noexcept {
  if (std::to_underlying(level::debug) <= k2::log_level_enabled()) {
    impl::select_logger_and_log(level::debug, std::nullopt, fmt, std::forward<Args>(args)...);
  }
}

template<typename... Args>
void trace(std::format_string<impl::wrapped_arg_t<Args>...> fmt, Args&&... args) noexcept {
  if (std::to_underlying(level::trace) <= k2::log_level_enabled()) {
    impl::select_logger_and_log(level::trace, std::nullopt, fmt, std::forward<Args>(args)...);
  }
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
