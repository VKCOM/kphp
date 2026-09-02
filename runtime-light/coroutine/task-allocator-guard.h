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
  kphp::coro::detail::memory::task_allocator& m_task_allocator{kphp::coro::detail::memory::task_allocator::get()};
  memory_resource::segmented_stack_resource<kphp::coro::detail::memory::task_allocator::shared_chunk_pool>* m_stack{nullptr};
  bool m_active{true};

public:
  task_allocator_guard() noexcept
      : m_stack{m_task_allocator.exchange_stack(nullptr)} {}

  explicit task_allocator_guard(kphp::coro::detail::memory::task_allocator& task_allocator) noexcept
      : m_task_allocator{task_allocator},
        m_stack{m_task_allocator.exchange_stack(nullptr)} {}

  task_allocator_guard(task_allocator_guard&& other) noexcept
      : m_task_allocator{other.m_task_allocator},
        m_stack{other.m_stack},
        m_active{std::exchange(other.m_active, false)} {}

  auto task_allocator() const noexcept -> kphp::coro::detail::memory::task_allocator& {
    return m_task_allocator;
  }

  ~task_allocator_guard() {
    if (m_active) {
      m_task_allocator.set_stack(m_stack);
    }
  }
};

} // namespace kphp::coro
