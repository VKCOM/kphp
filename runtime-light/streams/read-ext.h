// Compiler for PHP (aka KPHP)
// Copyright (c) 2025 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cstddef>
#include <span>

#include "runtime-common/core/allocator/script-allocator.h"
#include "runtime-common/core/runtime-core.h"
#include "runtime-common/core/std/containers.h"

namespace kphp::component::read_ext {

inline auto append(string& str) noexcept {
  return [&str](std::span<const std::byte> chunk) noexcept { str.append(reinterpret_cast<const char*>(chunk.data()), chunk.size()); };
}

inline auto append(kphp::stl::vector<std::byte, kphp::memory::script_allocator>& vec) noexcept {
  return [&vec](std::span<const std::byte> chunk) noexcept { vec.append_range(chunk); };
}

} // namespace kphp::component::read_ext
