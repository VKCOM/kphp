// Compiler for PHP (aka KPHP)
// Copyright (c) 2026 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime-light/coroutine/detail/allocator/task-allocator.h"

#include "runtime-light/coroutine/coroutine-state.h"

namespace kphp::coro::detail::memory {

auto task_allocator::get() noexcept -> task_allocator& {
  return kphp::coro::instance_state::get().task_allocator;
}

} // namespace kphp::coro::detail::memory
