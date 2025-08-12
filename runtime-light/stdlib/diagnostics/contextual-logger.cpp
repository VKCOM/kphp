// Compiler for PHP (aka KPHP)
// Copyright (c) 2025 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime-light/stdlib/diagnostics/contextual-logger.h"

#include <cstddef>
#include <format>
#include <optional>
#include <span>
#include <string_view>
#include <utility>

#include "runtime-common/core/allocator/script-allocator.h"
#include "runtime-common/core/std/containers.h"
#include "runtime-light/k2-platform/k2-api.h"
#include "runtime-light/state/component-state.h"
#include "runtime-light/state/image-state.h"
#include "runtime-light/state/instance-state.h"
#include "runtime-light/stdlib/diagnostics/backtrace.h"
#include "runtime-light/stdlib/diagnostics/detail/logs.h"

namespace kphp::log {

std::optional<std::reference_wrapper<contextual_logger>> contextual_logger::try_get() noexcept {
  if (const auto* instance_state_ptr{k2::instance_state()}; instance_state_ptr != nullptr) [[likely]] {
    return const_cast<contextual_logger&>(instance_state_ptr->instance_logger);
  } else if (const auto* component_state_ptr{k2::component_state()}; component_state_ptr != nullptr) {
    return const_cast<contextual_logger&>(component_state_ptr->component_logger);
  } else if (const auto* image_state_ptr{k2::image_state()}; image_state_ptr != nullptr) {
    return const_cast<contextual_logger&>(image_state_ptr->image_logger);
  }
  return std::nullopt;
}

void contextual_logger::log_with_tags(kphp::log::level level, std::optional<std::span<void* const>> trace, std::string_view message) const noexcept {
  kphp::stl::vector<k2::LogTaggedEntry, kphp::memory::script_allocator> tagged_entries{};
  if (level == level::warn || level == level::error) {
    tagged_entries.reserve(extra_tags.size());
    for (const auto& [key, value] : extra_tags) {
      tagged_entries.push_back(k2::LogTaggedEntry{.key = key.data(), .value = value.data(), .key_len = key.size(), .value_len = value.size()});
    }
  }
  if (trace.has_value()) {
    static constexpr std::string_view BACKTRACE_KEY = "trace";
    static constexpr size_t BACKTRACE_BUFFER_SIZE = 1024UZ * 4UZ;
    const auto& raw_backtrace{*trace};
    std::array<char, BACKTRACE_BUFFER_SIZE> backtrace_buffer; // NOLINT
    std::string_view backtrace{"[]"};
    if (auto backtrace_symbols{kphp::diagnostic::backtrace_symbols(raw_backtrace)}; !backtrace_symbols.empty()) {
      const auto [trace_out, trace_size]{std::format_to_n(backtrace_buffer.data(), backtrace_buffer.size() - 1, "\n{}", backtrace_symbols)};
      *trace_out = '\0';
      backtrace = std::string_view{backtrace_buffer.data(), static_cast<std::string_view::size_type>(trace_size)};
    } else if (auto backtrace_addresses{kphp::diagnostic::backtrace_addresses(raw_backtrace)}; !backtrace_addresses.empty()) {
      const auto [trace_out, trace_size]{std::format_to_n(backtrace_buffer.data(), backtrace_buffer.size() - 1, "{}", backtrace_addresses)};
      *trace_out = '\0';
      backtrace = std::string_view{backtrace_buffer.data(), static_cast<std::string_view::size_type>(trace_size)};
    }
    tagged_entries.push_back(
        k2::LogTaggedEntry{.key = BACKTRACE_KEY.data(), .value = backtrace.data(), .key_len = BACKTRACE_KEY.size(), .value_len = backtrace.size()});
    k2::log(std::to_underlying(level), message, tagged_entries);
    return;
  }
  k2::log(std::to_underlying(level), message, tagged_entries);
}
} // namespace kphp::log
