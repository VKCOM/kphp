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
#include <string_view>
#include <type_traits>
#include <utility>

#include "runtime-light/k2-platform/k2-api.h"
#include "runtime-light/stdlib/diagnostics/backtrace.h"

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

template<typename... Args>
void log(level level, std::optional<std::span<void* const>> trace, std::format_string<impl::wrapped_arg_t<Args>...> fmt, Args&&... args) noexcept {
  if (std::to_underlying(level) > k2::log_level_enabled()) {
    return;
  }

  static constexpr size_t LOG_BUFFER_SIZE = 512;
  std::array<char, LOG_BUFFER_SIZE> log_buffer;
  auto [out, size]{std::format_to_n<decltype(log_buffer.data()), impl::wrapped_arg_t<Args>...>(log_buffer.data(), log_buffer.size() - 1, fmt,
                                                                                               impl::wrap_log_argument(std::forward<Args>(args))...)};
  *out = '\0';
  auto message{std::string_view{log_buffer.data(), static_cast<std::string_view::size_type>(size)}};
  if (!trace.has_value()) {
    k2::log(std::to_underlying(level), message, std::nullopt);
    return;
  }

  static constexpr std::string_view backtrace_key = "trace";
  static constexpr size_t BACKTRACE_BUFFER_SIZE = 1024UZ * 4UZ;
  std::array<char, BACKTRACE_BUFFER_SIZE> backtrace_buffer;
  std::string_view backtrace;
  if (auto backtrace_symbols{kphp::diagnostic::backtrace_symbols(*trace)}; !backtrace_symbols.empty()) {
    const auto [trace_out, trace_size]{std::format_to_n(backtrace_buffer.data(), backtrace_buffer.size() - 1, "\n{}", backtrace_symbols)};
    *trace_out = '\0';
    backtrace = std::string_view{backtrace_buffer.data(), static_cast<std::string_view::size_type>(trace_size)};
  } else if (auto backtrace_addresses{kphp::diagnostic::backtrace_addresses(*trace)}; !backtrace_addresses.empty()) {
    const auto [trace_out, trace_size]{std::format_to_n(backtrace_buffer.data(), backtrace_buffer.size() - 1, "{}", backtrace_addresses)};
    *trace_out = '\0';
    backtrace = std::string_view{backtrace_buffer.data(), static_cast<std::string_view::size_type>(trace_size)};
  } else {
    backtrace = "can't resolve trace";
  }
  std::array<k2::LogTaggedEntry, 1> kv_pairs = {
      {k2::LogTaggedEntry{.key = backtrace_key.data(), .value = backtrace.data(), .key_len = backtrace_key.size(), .value_len = backtrace.size()}}};
  k2::log(std::to_underlying(level), message, std::span(kv_pairs.data(), kv_pairs.size()));
}

template<typename... Args>
void log_with_backtrace(level level, std::format_string<impl::wrapped_arg_t<Args>...> fmt, Args&&... args) noexcept {
  static constexpr size_t MAX_BACKTRACE_SIZE = 64;
  if (std::to_underlying(level) > k2::log_level_enabled()) {
    return;
  }

  std::array<void*, MAX_BACKTRACE_SIZE> backtrace{};
  const size_t num_frames{kphp::diagnostic::backtrace(backtrace)};
  const std::span<void* const> backtrace_view{backtrace.data(), num_frames};
  impl::log(level, backtrace_view, fmt, std::forward<Args>(args)...);
}
} // namespace impl

// The backtrace algorithm relies on the fact that assertion does not call backtrace.
// If assertion is modified, the backtrace algorithm should be updated accordingly
inline void assertion(bool condition, const std::source_location& location = std::source_location::current()) noexcept {
  if (!condition) [[unlikely]] {
    impl::log(impl::level::error, std::nullopt, "assertion failed at {}:{}", location.file_name(), location.line());
    k2::exit(1);
  }
}

template<typename... Args>
[[noreturn]] void error(std::format_string<impl::wrapped_arg_t<Args>...> fmt, Args&&... args) noexcept {
  impl::log_with_backtrace(impl::level::error, fmt, std::forward<Args>(args)...);
  k2::exit(1);
}

template<typename... Args>
void warning(std::format_string<impl::wrapped_arg_t<Args>...> fmt, Args&&... args) noexcept {
  impl::log_with_backtrace(impl::level::warn, fmt, std::forward<Args>(args)...);
}

template<typename... Args>
void info(std::format_string<impl::wrapped_arg_t<Args>...> fmt, Args&&... args) noexcept {
  impl::log(impl::level::info, std::nullopt, fmt, std::forward<Args>(args)...);
}

template<typename... Args>
void debug(std::format_string<impl::wrapped_arg_t<Args>...> fmt, Args&&... args) noexcept {
  impl::log(impl::level::debug, std::nullopt, fmt, std::forward<Args>(args)...);
}

template<typename... Args>
void trace(std::format_string<impl::wrapped_arg_t<Args>...> fmt, Args&&... args) noexcept {
  impl::log(impl::level::trace, std::nullopt, fmt, std::forward<Args>(args)...);
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
