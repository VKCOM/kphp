// Compiler for PHP (aka KPHP)
// Copyright (c) 2026 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <memory>

#include "common/mixin/not_copyable.h"
#include "runtime-common/core/memory-resource/segmented-stack-resource.h"
#include "runtime-light/coroutine/detail/allocator/task-allocator.h"

namespace kphp::coro {

struct root_promise : private vk::not_copyable {
private:
  memory_resource::segmented_stack_resource<kphp::coro::detail::memory::task_allocator::shared_chunk_pool> m_task_memory_resource;

public:
  root_promise() noexcept {
    kphp::coro::detail::memory::task_allocator::get().init_resource(std::addressof(m_task_memory_resource));
  }

  auto get_task_memory_resource() noexcept -> memory_resource::segmented_stack_resource<kphp::coro::detail::memory::task_allocator::shared_chunk_pool>& {
    return m_task_memory_resource;
  }
};

} // namespace kphp::coro
