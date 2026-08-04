// Compiler for PHP (aka KPHP)
// Copyright (c) 2026 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cstddef>

#include "common/mixin/not_copyable.h"
#include "runtime-light/allocator/allocator-state.h"
#include "runtime-light/coroutine/coroutine-state.h"
#include "runtime-light/coroutine/io-scheduler.h"
#include "runtime-light/coroutine/task.h"
#include "runtime-light/k2-platform/k2-api.h"
#include "runtime-light/stdlib/diagnostics/contextual-tags.h"

struct InstanceState final : vk::not_copyable {
  AllocatorState instance_allocator_state{INIT_INSTANCE_ALLOCATOR_SIZE, DEFAULT_MIN_EXTRA_MEMORY_POOL_SIZE, 0};

  kphp::log::contextual_tags instance_tags;

  kphp::coro::instance_state coroutine_instance_state;
  kphp::coro::io_scheduler io_scheduler{coroutine_instance_state};

  InstanceState() noexcept = default;
  static auto get() noexcept -> InstanceState&;

  auto init() noexcept -> void;

private:
  static constexpr auto INIT_INSTANCE_ALLOCATOR_SIZE = static_cast<size_t>(16U * 1024U * 1024U); // 16MiB

  auto run() noexcept -> kphp::coro::task<>;
};

inline auto InstanceState::get() noexcept -> InstanceState& {
  return *k2::instance_state();
}
