// Compiler for PHP (aka KPHP)
// Copyright (c) 2026 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <utility>

#include "common/mixin/not_copyable.h"
#include "runtime-common/core/memory-resource/segmented-stack-resource.h"
#include "runtime-light/coroutine/detail/allocator/task-allocator.h"

namespace kphp::coro {

class task_allocator_guard : private vk::not_copyable {
private:
  memory_resource::segmented_stack_resource<kphp::coro::detail::memory::task_allocator::shared_chunk_pool>* m_stack{nullptr};
  bool m_active{true};

public:
  task_allocator_guard() noexcept
      : m_stack{kphp::coro::detail::memory::task_allocator::get().exchange_stack(nullptr)} {}

  task_allocator_guard(task_allocator_guard&& other) noexcept
      : m_stack{other.m_stack},
        m_active{std::exchange(other.m_active, false)} {}

  ~task_allocator_guard() {
    if (m_active) {
      kphp::coro::detail::memory::task_allocator::get().set_stack(m_stack);
    }
  }
};

} // namespace kphp::coro
