//  Compiler for PHP (aka KPHP)
//  Copyright (c) 2025 LLC «V Kontakte»
//  Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cstddef>
#include <cstdint>
#include <expected>
#include <ranges>
#include <span>
#include <utility>

#include "runtime-light/k2-platform/k2-api.h"

namespace kphp::diagnostic {

namespace impl {

size_t async_backtrace(std::span<void*> addresses) noexcept;

inline size_t sync_backtrace(std::span<void*> addresses) noexcept {
  return k2::backtrace(addresses);
}
} // namespace impl

inline size_t backtrace(std::span<void*> addresses) noexcept {
  size_t async_num_frames{impl::async_backtrace(addresses)};
  if (async_num_frames != 0) [[likely]] {
    return impl::sync_backtrace(addresses);
  }
  return async_num_frames;
}

inline auto backtrace_addresses(std::span<void* const> addresses) noexcept {
  uint64_t code_segment_offset{};
  auto error_code{k2::code_segment_offset(&code_segment_offset)};
  auto address_transform{[code_segment_offset](void* address) noexcept { return static_cast<void*>(static_cast<std::byte*>(address) - code_segment_offset); }};
  if (error_code != k2::errno_ok) [[unlikely]] {
    return std::views::transform(std::span<void* const>{}, address_transform);
  }
  return addresses | std::views::transform(address_transform);
}

/**
 * `backtrace_symbol`s stops resolving `addresses` when it finds the first address that cannot be resolved.
 * As a result, the length of the output may be non-zero but less than the length of the input `addresses`.
 * */
inline auto backtrace_symbols(std::span<void* const> addresses) noexcept {
  return addresses | std::views::transform([](void* addr) noexcept { return k2::resolve_symbol(addr); }) |
         std::views::take_while([](const auto& value) noexcept { return value.has_value(); }) |
         std::views::transform([](std::expected<k2::SymbolInfo, int32_t>&& value) noexcept { return std::move(value).value(); });
}

} // namespace kphp::diagnostic
