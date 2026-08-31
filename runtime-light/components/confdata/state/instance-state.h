// Compiler for PHP (aka KPHP)
// Copyright (c) 2026 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cstddef>
#include <cstdint>

#include "common/mixin/not_copyable.h"
#include "runtime-light/allocator/allocator-state.h"
#include "runtime-light/components/confdata/confdata-proxy/sync-functions.h"
#include "runtime-light/coroutine/coroutine-state.h"
#include "runtime-light/coroutine/io-scheduler.h"
#include "runtime-light/coroutine/task.h"
#include "runtime-light/k2-platform/k2-api.h"
#include "runtime-light/stdlib/diagnostics/contextual-tags.h"

struct InstanceState final : vk::not_copyable {
  enum class warmup_status : uint8_t { pending, done };

  AllocatorState m_allocator_state{INIT_INSTANCE_ALLOCATOR_SIZE, DEFAULT_MIN_INSTANCE_EXTRA_MEMORY_POOL_SIZE, 0};

  warmup_status m_warmup_status{warmup_status::pending};
  kphp::confdata::pagination m_pagination{};

  kphp::log::contextual_tags m_instance_tags;

  kphp::coro::instance_state m_coroutine_instance_state{INIT_INSTANCE_COROUTINE_ALLOCATOR_SIZE,
                                                        DEFAULT_MIN_INSTANCE_EXTRA_COROUTINE_MEMORY_POOL_SIZE,
                                                        0,
                                                        INIT_INSTANCE_TASK_ALLOCATOR_SIZE,
                                                        DEFAULT_TASK_ALLOCATOR_SEGMENT_SIZE,
                                                        DEFAULT_INSTANCE_TASK_ALLOCATOR_STACK_POOL_CHUNK_SIZE,
                                                        DEFAULT_MIN_EXTRA_TASK_MEMORY_POOL_SIZE,
                                                        0};
  kphp::coro::io_scheduler m_io_scheduler{m_coroutine_instance_state};

  InstanceState() noexcept = default;
  static auto get() noexcept -> InstanceState&;

  auto init() noexcept -> void;

private:
  static constexpr auto INIT_INSTANCE_ALLOCATOR_SIZE = static_cast<size_t>(16U * 1024U * 1024U);                   // 16MiB
  static constexpr auto DEFAULT_MIN_INSTANCE_EXTRA_MEMORY_POOL_SIZE = static_cast<size_t>(1024U * 1024U);          // 1MiB
  static constexpr auto INIT_INSTANCE_COROUTINE_ALLOCATOR_SIZE = static_cast<size_t>(2U * 1024U * 1024U);          // 2MiB
  static constexpr auto DEFAULT_MIN_INSTANCE_EXTRA_COROUTINE_MEMORY_POOL_SIZE = static_cast<size_t>(512U * 1024U); // 0.5MiB
  static constexpr auto INIT_INSTANCE_TASK_ALLOCATOR_SIZE = static_cast<size_t>(512U * 1024U);                     // 512KiB
  static constexpr auto DEFAULT_TASK_ALLOCATOR_SEGMENT_SIZE = static_cast<size_t>(32U * 1024U);                    // 32KiB
  static constexpr auto DEFAULT_INSTANCE_TASK_ALLOCATOR_STACK_POOL_CHUNK_SIZE = static_cast<size_t>(60U);
  static constexpr auto DEFAULT_MIN_EXTRA_TASK_MEMORY_POOL_SIZE = static_cast<size_t>(128U * 1024U); // 128 KiB

  auto run() noexcept -> kphp::coro::task<>;
  auto accept_loop() noexcept -> kphp::coro::task<>;
  auto service_loop() noexcept -> kphp::coro::task<>;
};

inline auto InstanceState::get() noexcept -> InstanceState& {
  return *k2::instance_state();
}
