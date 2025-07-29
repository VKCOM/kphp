// Compiler for PHP (aka KPHP)
// Copyright (c) 2025 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime-light/stdlib/diagnostics/logs.h"

#include <array>
#include <format>
#include <optional>
#include <span>
#include <string_view>
#include <utility>

#include "runtime-light/k2-platform/k2-api.h"
#include "runtime-light/state/image-state.h"
#include "runtime-light/stdlib/diagnostics/backtrace.h"

namespace kphp::log::impl {

void log(level level, std::optional<std::span<void* const>> trace, std::string_view message) noexcept {
  std::array<k2::LogTaggedEntry, 2> tagged_entries;

  static constexpr std::string_view version_key = "version";
  std::string_view version_value{
      ImageState::try_get().transform([](const ImageState& image_st) { return std::string_view{image_st.image_version}; }).value_or("1000")};
  tagged_entries[0] =
      k2::LogTaggedEntry{.key = version_key.data(), .value = version_value.data(), .key_len = version_key.size(), .value_len = version_value.size()};

  if (!trace.has_value()) {
    k2::log(std::to_underlying(level), message, std::span(tagged_entries.data(), 1));
    return;
  }

  static constexpr std::string_view backtrace_key = "trace";
  static constexpr size_t BACKTRACE_BUFFER_SIZE = 1024UZ * 4UZ;
  std::array<char, BACKTRACE_BUFFER_SIZE> backtrace_buffer;
  std::string_view backtrace{"[]"};
  if (auto backtrace_symbols{kphp::diagnostic::backtrace_symbols(*trace)}; !backtrace_symbols.empty()) {
    const auto [trace_out, trace_size]{std::format_to_n(backtrace_buffer.data(), backtrace_buffer.size() - 1, "\n{}", backtrace_symbols)};
    *trace_out = '\0';
    backtrace = std::string_view{backtrace_buffer.data(), static_cast<std::string_view::size_type>(trace_size)};
  } else if (auto backtrace_addresses{kphp::diagnostic::backtrace_addresses(*trace)}; !backtrace_addresses.empty()) {
    const auto [trace_out, trace_size]{std::format_to_n(backtrace_buffer.data(), backtrace_buffer.size() - 1, "{}", backtrace_addresses)};
    *trace_out = '\0';
    backtrace = std::string_view{backtrace_buffer.data(), static_cast<std::string_view::size_type>(trace_size)};
  }
  tagged_entries[1] =
      k2::LogTaggedEntry{.key = backtrace_key.data(), .value = backtrace.data(), .key_len = backtrace_key.size(), .value_len = backtrace.size()};
  k2::log(std::to_underlying(level), message, tagged_entries);
}

} // namespace kphp::log::impl
