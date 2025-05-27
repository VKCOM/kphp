//  Compiler for PHP (aka KPHP)
//  Copyright (c) 2025 LLC «V Kontakte»
//  Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cstddef>
#include <cstdint>
#include <ranges>
#include <span>

#include "runtime-light/k2-platform/k2-api.h"

namespace kphp::diagnostic {

size_t backtrace(std::span<void*> addresses) noexcept;

inline auto backtrace_code_address(std::span<void* const> addresses) noexcept {
  static constexpr std::span<void* const> empty_span{};
  uint64_t code_segment_offset{};
  auto error_code{k2::code_segment_offset(&code_segment_offset)};
  auto address_transform{[code_segment_offset](void* address) noexcept { return static_cast<void*>(static_cast<std::byte*>(address) - code_segment_offset); }};
  if (error_code != k2::errno_ok) [[unlikely]] {
    return std::views::transform(empty_span, address_transform);
  }
  return addresses | std::views::transform(address_transform);
}

} // namespace kphp::diagnostic
