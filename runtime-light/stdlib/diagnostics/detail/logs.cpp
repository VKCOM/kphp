// Compiler for PHP (aka KPHP)
// Copyright (c) 2026 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime-light/stdlib/diagnostics/detail/logs.h"

#include <cstddef>
#include <format>
#include <iterator>
#include <span>
#include <string_view>

#include "runtime-light/stdlib/diagnostics/backtrace.h"

namespace kphp::log::impl {

namespace {

struct truncating_iterator {
  using iterator_category = std::output_iterator_tag;
  using value_type = void;
  using difference_type = std::ptrdiff_t;
  using pointer = void;
  using reference = void;

  char* dst;
  size_t max_size;
  size_t count{};
  mutable char blackhole{};

  truncating_iterator(char* out, size_t max_size) noexcept
      : dst{out},
        max_size{max_size} {}

  truncating_iterator& operator++() noexcept {
    if (count++ < max_size) {
      ++dst;
    }
    return *this;
  }

  truncating_iterator operator++(int) noexcept { // NOLINT
    auto it = *this;
    ++*this;
    return it;
  }

  char& operator*() noexcept {
    return count < max_size ? *dst : blackhole;
  }
};

} // namespace

size_t resolve_log_trace(std::span<char> trace_buffer, std::span<void* const> raw_trace) noexcept {
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

size_t format_log_message(std::span<char> message_buffer, std::string_view fmt, std::format_args args) noexcept {
  auto it{std::vformat_to(truncating_iterator{message_buffer.data(), message_buffer.size() - 1}, fmt, args)};
  *it.dst = '\0';
  return std::distance(message_buffer.data(), it.dst);
}

} // namespace kphp::log::impl
