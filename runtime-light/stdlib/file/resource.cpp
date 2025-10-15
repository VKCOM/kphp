// Compiler for PHP (aka KPHP)
// Copyright (c) 2025 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime-light/stdlib/file/resource.h"

#include <cstdint>
#include <expected>

#include "runtime-light/state/instance-state.h"

namespace kphp::fs {

auto stdoutput::open() noexcept -> std::expected<stdoutput, int32_t> {
  if (InstanceState::get().instance_kind() != instance_kind::cli) [[unlikely]] {
    return std::unexpected{k2::errno_einval};
  }
  return {stdoutput{}};
}

auto input::open() noexcept -> std::expected<input, int32_t> {
  if (InstanceState::get().instance_kind() != instance_kind::http_server) [[unlikely]] {
    return std::unexpected{k2::errno_einval};
  }
  return {input{}};
}

} // namespace kphp::fs
