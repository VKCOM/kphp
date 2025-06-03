// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <array>
#include <cstddef>
#include <format>
#include <source_location>
#include <span>
#include <type_traits>
#include <utility>

#include "runtime-light/k2-platform/k2-api.h"
#include "runtime-light/stdlib/diagnostics/backtrace.h"

enum class LogLevel : size_t {
  Error = 1,
  Warn = 2,
  Info = 3,
  Debug = 4,
  Trace = 5,
};

namespace kphp::log {

namespace impl {

template<typename T>
requires std::is_floating_point_v<std::remove_cvref_t<T>>
struct floating_wrapper final {
  T value{};
};

template<typename T>
constexpr auto wrap_log_argument(T&& t) noexcept -> decltype(auto) {
  if constexpr (std::is_floating_point_v<std::remove_cvref_t<T>>) {
    return impl::floating_wrapper<std::remove_cvref_t<T>>{.value = std::forward<T>(t)};
  } else {
    return std::forward<T>(t);
  }
}

template<typename T>
using wrapped_arg_t = std::invoke_result_t<decltype(impl::wrap_log_argument<T>), T>;

enum class level : size_t { error = 1, warn, info, debug, trace };

template<size_t LOG_BUFFER_SIZE, typename BACKTRACE_T, typename... Args>
void write_log(level level, BACKTRACE_T backtrace, std::format_string<impl::wrapped_arg_t<Args>...> fmt, Args&&... args) {
  std::array<char, LOG_BUFFER_SIZE> log_buffer;
  auto [out, size]{std::format_to_n<decltype(log_buffer.data()), impl::wrapped_arg_t<Args>...>(log_buffer.data(), log_buffer.size() - 1, fmt,
                                                                                               impl::wrap_log_argument(std::forward<Args>(args))...)};

  if (level < level::debug) {
    const auto res{std::format_to_n(out, std::distance(out, log_buffer.end()) - 1, "\nBacktrace\n{}", backtrace)};
    out = res.out;
    size += res.size;
  }
  *out = '\0';
  k2::log(std::to_underlying(level), size, log_buffer.data());
}

template<typename... Args>
void log(level level, std::format_string<impl::wrapped_arg_t<Args>...> fmt, Args&&... args) noexcept {
  if (std::to_underlying(level) > k2::log_level_enabled()) {
    return;
  }

  static constexpr size_t MAX_BACKTRACE_SIZE = 64;

  std::array<void*, MAX_BACKTRACE_SIZE> backtrace{};
  const size_t num_frames{kphp::diagnostic::backtrace(backtrace)};
  std::span<void* const> backtrace_view{backtrace.data(), num_frames};

  static constexpr size_t SMALL_BUFFER_SIZE = 512;
  static constexpr size_t BIG_BUFFER_SIZE = 1024 * 4;

  if (auto backtrace_symbols{kphp::diagnostic::backtrace_symbols(backtrace_view)}; !backtrace_symbols.empty()) {
    return write_log<BIG_BUFFER_SIZE>(level, backtrace_symbols, fmt, std::forward<Args>(args)...);
  }
  if (auto backtrace_addresses{kphp::diagnostic::backtrace_addresses(backtrace_view)}; !backtrace_addresses.empty()) {
    return write_log<SMALL_BUFFER_SIZE>(level, backtrace_addresses, fmt, std::forward<Args>(args)...);
  }
  return write_log<SMALL_BUFFER_SIZE>(level, "cannot resolve virtual addresses to debug information", fmt, std::forward<Args>(args)...);
}
} // namespace impl

template<typename... Args>
[[noreturn]] void error(std::format_string<impl::wrapped_arg_t<Args>...> fmt, Args&&... args) noexcept {
  impl::log(impl::level::error, fmt, std::forward<Args>(args)...);
  k2::exit(1);
}

// The backtrace algorithm relies on the fact that assertion does not call backtrace.
// If assertion is modified, the backtrace algorithm should be updated accordingly
inline void assertion(bool condition, const std::source_location& location = std::source_location::current()) noexcept {
  if (!condition) [[unlikely]] {
    static constexpr size_t LOG_BUFFER_SIZE = 512;
    std::array<char, LOG_BUFFER_SIZE> log_buffer;
    auto [out, size]{std::format_to_n(log_buffer.data(), log_buffer.size() - 1, "assertion failed at {}:{}", location.file_name(), location.line())};
    *out = '\0';
    k2::log(std::to_underlying(impl::level::error), size, log_buffer.data());
    k2::exit(1);
  }
}

template<typename... Args>
void warning(std::format_string<impl::wrapped_arg_t<Args>...> fmt, Args&&... args) noexcept {
  impl::log(impl::level::warn, fmt, std::forward<Args>(args)...);
}

template<typename... Args>
void info(std::format_string<impl::wrapped_arg_t<Args>...> fmt, Args&&... args) noexcept {
  impl::log(impl::level::info, fmt, std::forward<Args>(args)...);
}

template<typename... Args>
void debug(std::format_string<impl::wrapped_arg_t<Args>...> fmt, Args&&... args) noexcept {
  impl::log(impl::level::debug, fmt, std::forward<Args>(args)...);
}

template<typename... Args>
void trace(std::format_string<impl::wrapped_arg_t<Args>...> fmt, Args&&... args) noexcept {
  impl::log(impl::level::trace, fmt, std::forward<Args>(args)...);
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
