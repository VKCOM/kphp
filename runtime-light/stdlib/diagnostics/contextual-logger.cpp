// Compiler for PHP (aka KPHP)
// Copyright (c) 2025 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime-light/stdlib/diagnostics/contextual-logger.h"

#include <cstddef>
#include <functional>
#include <optional>
#include <span>
#include <string_view>
#include <utility>

#include "runtime-common/core/allocator/script-allocator.h"
#include "runtime-common/core/std/containers.h"
#include "runtime-light/k2-platform/k2-api.h"
#include "runtime-light/state/instance-state.h"
#include "runtime-light/stdlib/diagnostics/detail/logs.h"

namespace kphp::log {

std::optional<std::reference_wrapper<contextual_logger>> contextual_logger::try_get() noexcept {
  if (auto* instance_state_ptr{k2::instance_state()}; instance_state_ptr != nullptr) [[likely]] {
    return instance_state_ptr->instance_logger;
  }
  return std::nullopt;
}

void contextual_logger::log_with_tags(kphp::log::level level, std::optional<std::span<void* const>> trace, std::string_view message) const noexcept {
  kphp::stl::vector<k2::LogTaggedEntry, kphp::memory::script_allocator> tagged_entries{};
  const bool is_extra_tags_enabled{level == level::warn || level == level::error};
  const size_t tagged_entries_size{static_cast<size_t>((trace.has_value() ? 1 : 0) + (is_extra_tags_enabled ? extra_tags.size() : 0))};
  tagged_entries.reserve(tagged_entries_size);
  if (is_extra_tags_enabled) {
    for (const auto& [key, value] : extra_tags) {
      tagged_entries.push_back(k2::LogTaggedEntry{.key = key.data(), .value = value.data(), .key_len = key.size(), .value_len = value.size()});
    }
  }
  if (trace.has_value()) {
    static constexpr size_t BACKTRACE_BUFFER_SIZE = 1024UZ * 4UZ;
    std::array<char, BACKTRACE_BUFFER_SIZE> backtrace_buffer; // NOLINT
    size_t backtrace_size{impl::resolve_log_trace(backtrace_buffer, *trace)};
    tagged_entries.push_back(
        k2::LogTaggedEntry{.key = BACKTRACE_KEY.data(), .value = backtrace_buffer.data(), .key_len = BACKTRACE_KEY.size(), .value_len = backtrace_size});
    k2::log(std::to_underlying(level), message, tagged_entries);
    return;
  }
  k2::log(std::to_underlying(level), message, tagged_entries);
}

} // namespace kphp::log
