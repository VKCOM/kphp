// Compiler for PHP (aka KPHP)
// Copyright (c) 2026 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "common/mixin/not_copyable.h"
#include "runtime-common/core/memory-resource/segmented-stack-resource.h"
#include "runtime-light/coroutine/detail/allocator/task-allocator.h"

namespace kphp::coro {

class task_allocator_guard : private vk::not_copyable {
private:
  memory_resource::segmented_stack_resource<kphp::coro::detail::memory::task_allocator::shared_chunk_pool>* m_resource{nullptr};

public:
  task_allocator_guard() noexcept
      : m_resource{kphp::coro::detail::memory::task_allocator::get().exchange(nullptr)} {}

  ~task_allocator_guard() {
    kphp::coro::detail::memory::task_allocator::get().set(m_resource);
  }
};

} // namespace kphp::coro
