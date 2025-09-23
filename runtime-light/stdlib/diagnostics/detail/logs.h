// Compiler for PHP (aka KPHP)
// Copyright (c) 2025 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cstddef>
#include <cstring>
#include <format>
#include <iterator>
#include <span>
#include <string_view>
#include <type_traits>

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

template<typename... Args>
size_t format_log_message(std::span<char> message_buffer, std::format_string<impl::wrapped_arg_t<Args>...> fmt, Args&&... args) noexcept {
  auto [out, _]{std::format_to_n<decltype(message_buffer.data()), impl::wrapped_arg_t<Args>...>(message_buffer.data(), message_buffer.size() - 1, fmt,
                                                                                                impl::wrap_log_argument(std::forward<Args>(args))...)};
  *out = '\0';
  return std::distance(message_buffer.data(), out);
}

inline size_t resolve_log_trace(std::span<char> trace_buffer, std::span<void* const> raw_trace) noexcept {
  if (trace_buffer.empty()) {
    return 0;
  }

  if (auto backtrace_symbols{kphp::diagnostic::backtrace_symbols(raw_trace)}; !backtrace_symbols.empty()) {
    const auto [trace_out, _]{std::format_to_n(trace_buffer.data(), trace_buffer.size() - 1, "\n{}", backtrace_symbols)};
    *trace_out = '\0';
    return std::distance(trace_buffer.data(), trace_out);
  } else if (auto backtrace_addresses{kphp::diagnostic::backtrace_addresses(raw_trace)}; !backtrace_addresses.empty()) {
    const auto [trace_out, _]{std::format_to_n(trace_buffer.data(), trace_buffer.size() - 1, "{}", backtrace_addresses)};
    *trace_out = '\0';
    return std::distance(trace_buffer.data(), trace_out);
  } else {
    static constexpr std::string_view DEFAULT_TRACE = "[]";
    const auto [trace_out, _]{std::format_to_n(trace_buffer.data(), trace_buffer.size() - 1, "{}", DEFAULT_TRACE)};
    *trace_out = '\0';
    return std::distance(trace_buffer.data(), trace_out);
  }
}

} // namespace impl

enum class level : size_t { error = 1, warn, info, debug, trace }; // NOLINT

} // namespace kphp::log
