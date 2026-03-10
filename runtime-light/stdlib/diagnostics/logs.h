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
#include "runtime-light/stdlib/diagnostics/contextual-tags.h"
#include "runtime-light/stdlib/diagnostics/detail/logs.h"
#include "runtime-light/stdlib/diagnostics/error-handling-state.h"

namespace kphp::log {

namespace impl {

template<typename... Args>
void log(level level, std::optional<std::span<void* const>> trace, std::format_string<impl::wrapped_arg_t<Args>...> fmt, Args&&... args) noexcept {
  static constexpr size_t LOG_BUFFER_SIZE = 2048UZ;
  std::array<char, LOG_BUFFER_SIZE> log_buffer; // NOLINT
  size_t message_size{impl::format_log_message(log_buffer, fmt, std::forward<Args>(args)...)};
  auto message{std::string_view{log_buffer.data(), static_cast<std::string_view::size_type>(message_size)}};

  auto opt_tags{
      contextual_tags::try_get().and_then([level](contextual_tags& tags) noexcept -> std::optional<std::reference_wrapper<kphp::log::contextual_tags>> {
        if (level == level::warn || level == level::error) {
          return std::make_optional(std::ref(tags));
        }
        return std::nullopt;
      })};

  const size_t tagged_entries_size{
      static_cast<size_t>((trace.has_value() ? 1 : 0) + opt_tags.transform([](contextual_tags& tags) noexcept { return tags.size(); }).value_or(0))};
  kphp::stl::vector<k2::LogTaggedEntry, kphp::memory::script_allocator> tagged_entries{};
  tagged_entries.reserve(tagged_entries_size);

  opt_tags.transform([&tagged_entries](contextual_tags& tags) noexcept {
    for (const auto& [key, value] : tags) {
      tagged_entries.push_back(k2::LogTaggedEntry{.key = key.data(), .value = value.data(), .key_len = key.size(), .value_len = value.size()});
    }
    return 0;
  });
  if (trace.has_value()) {
    static constexpr size_t BACKTRACE_BUFFER_SIZE = 1024UZ * 4UZ;
    static constexpr std::string_view BACKTRACE_KEY = "trace";
    std::array<char, BACKTRACE_BUFFER_SIZE> backtrace_buffer; // NOLINT
    size_t backtrace_size{impl::resolve_log_trace(backtrace_buffer, *trace)};
    tagged_entries.push_back(
        k2::LogTaggedEntry{.key = BACKTRACE_KEY.data(), .value = backtrace_buffer.data(), .key_len = BACKTRACE_KEY.size(), .value_len = backtrace_size});
  }

  k2::log(std::to_underlying(level), message, tagged_entries);
}

} // namespace impl

// The backtrace algorithm relies on the fact that assertion does not call backtrace.
// If assertion is modified, the backtrace algorithm should be updated accordingly
inline void assertion(bool condition, const std::source_location& location = std::source_location::current()) noexcept {
  if (!condition) [[unlikely]] {
    impl::log(level::error, std::nullopt, "assertion failed at {}:{}", location.file_name(), location.line());
    k2::exit(1);
  }
}

template<typename... Args>
[[noreturn]] void error(std::format_string<impl::wrapped_arg_t<Args>...> fmt, Args&&... args) noexcept {
  if (const auto& error_handling_state_opt{ErrorHandlingState::try_get()};
      error_handling_state_opt.has_value() && !(*error_handling_state_opt).get().log_level_enabled(E_ERROR)) {
  } else if (std::to_underlying(level::error) <= k2::log_level_enabled()) {
    std::array<void*, kphp::diagnostic::DEFAULT_BACKTRACE_MAX_SIZE> backtrace{};
    const size_t num_frames{kphp::diagnostic::backtrace(backtrace)};
    const std::span<void* const> backtrace_view{backtrace.data(), num_frames};
    impl::log(level::error, backtrace_view, fmt, std::forward<Args>(args)...);
  }
  k2::exit(1);
}

template<typename... Args>
void warning(std::format_string<impl::wrapped_arg_t<Args>...> fmt, Args&&... args) noexcept {
  if (const auto& error_handling_state_opt{ErrorHandlingState::try_get()};
      error_handling_state_opt.has_value() && !(*error_handling_state_opt).get().log_level_enabled(E_WARNING)) {
    return;
  }
  if (std::to_underlying(level::warn) <= k2::log_level_enabled()) {
    std::array<void*, kphp::diagnostic::DEFAULT_BACKTRACE_MAX_SIZE> backtrace{};
    const size_t num_frames{kphp::diagnostic::backtrace(backtrace)};
    const std::span<void* const> backtrace_view{backtrace.data(), num_frames};
    impl::log(level::warn, backtrace_view, fmt, std::forward<Args>(args)...);
  }
}

template<typename... Args>
void info(std::format_string<impl::wrapped_arg_t<Args>...> fmt, Args&&... args) noexcept {
  if (const auto& error_handling_state_opt{ErrorHandlingState::try_get()};
      error_handling_state_opt.has_value() && !(*error_handling_state_opt).get().log_level_enabled(E_NOTICE)) {
    return;
  }
  if (std::to_underlying(level::info) <= k2::log_level_enabled()) {
    impl::log(level::info, std::nullopt, fmt, std::forward<Args>(args)...);
  }
}

template<typename... Args>
void debug(std::format_string<impl::wrapped_arg_t<Args>...> fmt, Args&&... args) noexcept {
  if (std::to_underlying(level::debug) <= k2::log_level_enabled()) {
    impl::log(level::debug, std::nullopt, fmt, std::forward<Args>(args)...);
  }
}

template<typename... Args>
void trace(std::format_string<impl::wrapped_arg_t<Args>...> fmt, Args&&... args) noexcept {
  if (std::to_underlying(level::trace) <= k2::log_level_enabled()) {
    impl::log(level::trace, std::nullopt, fmt, std::forward<Args>(args)...);
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
