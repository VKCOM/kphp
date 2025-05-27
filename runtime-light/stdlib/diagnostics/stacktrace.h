//  Compiler for PHP (aka KPHP)
//  Copyright (c) 2025 LLC «V Kontakte»
//  Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cstddef>
#include <cstdint>
#include <span>

#include "runtime-light/k2-platform/k2-api.h"

namespace kphp::diagnostic {

size_t async_backtrace(std::span<void*> addresses) noexcept;

inline bool resolve_static_offsets(std::span<void*> addresses) noexcept {
  uint64_t code_segment_offset{};
  if (auto error_code{k2::code_segment_offset(&code_segment_offset)}; error_code != k2::errno_ok) [[unlikely]] {
    return false;
  }
  for (auto& address : addresses) {
    address = reinterpret_cast<std::byte*>(address) - code_segment_offset;
  }
  return true;
}

} // namespace kphp::diagnostic
