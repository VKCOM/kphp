// Compiler for PHP (aka KPHP)
// Copyright (c) 2026 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <memory>
#include <new>

#include "common/mixin/not_copyable.h"
#include "runtime-common/core/memory-resource/segmented-stack-resource.h"
#include "runtime-light/coroutine/detail/allocator/coroutine-malloc-interface.h"
#include "runtime-light/coroutine/detail/allocator/task-allocator.h"
#include "runtime-light/stdlib/diagnostics/logs.h"

namespace kphp::coro {

/*
 * This scope is used to allocate task<T> coroutines with stack allocator. If you use this scope, you must keep 2 invariants:
 * 1) All operations for creating and destroying tasks within this scope must be performed in reverse order.
 * 2) All tasks created within this scope must be co_await-ed.
 */
class stack_scope : private vk::not_copyable {
private:
  bool m_owner{false};

public:
  stack_scope() noexcept {
    if (kphp::coro::detail::memory::task_allocator::get().current() == nullptr) {
      void* mem{kphp::coro::detail::memory::alloc_aligned(
          sizeof(memory_resource::segmented_stack_resource<kphp::coro::detail::memory::task_allocator::shared_chunk_pool>),
          static_cast<std::align_val_t>(alignof(memory_resource::segmented_stack_resource<kphp::coro::detail::memory::task_allocator::shared_chunk_pool>)))};

      kphp::log::assertion(mem != nullptr);

      auto* stack{
          std::construct_at(static_cast<memory_resource::segmented_stack_resource<kphp::coro::detail::memory::task_allocator::shared_chunk_pool>*>(mem))};
      kphp::coro::detail::memory::task_allocator::get().init_resource(stack);
      kphp::coro::detail::memory::task_allocator::get().set(stack);

      m_owner = true;
    }
  }

  ~stack_scope() {
    if (m_owner) {
      auto* stack{kphp::coro::detail::memory::task_allocator::get().current()};

      kphp::log::assertion(stack->empty());

      kphp::coro::detail::memory::task_allocator::get().set(nullptr);
      std::destroy_at(stack);
      kphp::coro::detail::memory::free(stack);
    }
  }
};

} // namespace kphp::coro
