//  Compiler for PHP (aka KPHP)
//  Copyright (c) 2026 LLC «V Kontakte»
//  Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime-light/stdlib/diagnostics/error-handling-functions.h"

#include <array>
#include <cstddef>
#include <format>
#include <iterator>
#include <span>
#include <utility>

#include "runtime-common/core/runtime-core.h"
#include "runtime-light/stdlib/diagnostics/backtrace.h"

array<array<string>> error_handling_impl_::format_backtrace_addresses(std::span<void* const> backtrace) noexcept {
  static constexpr size_t LOG_BUFFER_SIZE = 32;

  auto resolved_backtrace{kphp::diagnostic::backtrace_addresses(backtrace)};
  if (resolved_backtrace.empty()) {
    return {};
  }

  array<array<string>> backtrace_addresses{array_size{static_cast<int64_t>(backtrace.size()), true}};
  const string function_key{FUNCTION_KEY.data(), FUNCTION_KEY.size()};

  for (const auto& address : resolved_backtrace) {
    std::array<char, LOG_BUFFER_SIZE> log_buffer{};
    const auto [out, _]{std::format_to_n(log_buffer.data(), log_buffer.size() - 1, "{}", address)};
    *out = '\0';
    const auto recorded{std::distance(log_buffer.data(), out)};
    array<string> frame_info{array_size{1, false}};
    frame_info.set_value(function_key, string{log_buffer.data(), static_cast<string::size_type>(recorded)});

    backtrace_addresses.emplace_back(std::move(frame_info));
  }

  return backtrace_addresses;
}
