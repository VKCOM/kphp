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

class stack_scope : private vk::not_copyable {
private:
  memory_resource::segmented_stack_resource<kphp::coro::detail::memory::task_allocator::shared_chunk_pool>* m_stack{nullptr};
  bool m_owner{false};

public:
  stack_scope() noexcept
      : m_stack{kphp::coro::detail::memory::task_allocator::get().current()} {
    if (m_stack == nullptr) {
      m_stack = static_cast<memory_resource::segmented_stack_resource<kphp::coro::detail::memory::task_allocator::shared_chunk_pool>*>(
          kphp::coro::detail::memory::alloc_aligned(
              sizeof(memory_resource::segmented_stack_resource<kphp::coro::detail::memory::task_allocator::shared_chunk_pool>),
              static_cast<std::align_val_t>(
                  alignof(memory_resource::segmented_stack_resource<kphp::coro::detail::memory::task_allocator::shared_chunk_pool>))));

      kphp::log::assertion(m_stack != nullptr);

      m_stack = std::construct_at(m_stack);
      kphp::coro::detail::memory::task_allocator::get().init_resource(m_stack);
      kphp::coro::detail::memory::task_allocator::get().set(m_stack);

      m_owner = true;
    }
  }

  ~stack_scope() {
    if (m_owner) {
      kphp::log::assertion(m_stack->empty());

      kphp::coro::detail::memory::task_allocator::get().set(nullptr);
      std::destroy_at(m_stack);
      kphp::coro::detail::memory::free(m_stack);
    }
  }
};

} // namespace kphp::coro
