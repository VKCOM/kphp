// Compiler for PHP (aka KPHP)
// Copyright (c) 2025 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "common/mixin/not_copyable.h"
#include "runtime-light/coroutine/async-stack.h"
#include "runtime-light/coroutine/detail/allocator/runtime-coroutine-allocator.h"
#include "runtime-light/coroutine/detail/allocator/task-allocator.h"

namespace kphp::coro {

struct instance_state final : private vk::not_copyable {

  instance_state(size_t coroutine_mem_size, size_t min_extra_coroutine_mem_size, size_t oom_handling_coroutine_mem_size, size_t task_mem_size,
                 size_t task_allocator_segment_size, size_t task_allocator_stack_pool_chunk_size, size_t min_extra_task_mem_size,
                 size_t oom_handling_task_mem_size) noexcept
      : coroutine_allocator{coroutine_mem_size, min_extra_coroutine_mem_size, oom_handling_coroutine_mem_size},
        task_allocator{task_mem_size, task_allocator_segment_size, task_allocator_stack_pool_chunk_size, min_extra_task_mem_size, oom_handling_task_mem_size} {}

  static instance_state& get() noexcept;

  kphp::coro::detail::memory::RuntimeCoroutineAllocator coroutine_allocator;
  kphp::coro::detail::memory::task_allocator task_allocator;
  kphp::coro::async_stack_root coroutine_stack_root;
};

} // namespace kphp::coro
