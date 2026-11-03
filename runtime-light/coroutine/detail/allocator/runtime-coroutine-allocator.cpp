// Compiler for PHP (aka KPHP)
// Copyright (c) 2026 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime-light/coroutine/detail/allocator/runtime-coroutine-allocator.h"

#include "runtime-light/coroutine/coroutine-state.h"

namespace kphp::coro::detail::memory {

auto runtime_coroutine_allocator::get() noexcept -> runtime_coroutine_allocator& {
  return kphp::coro::instance_state::get().coroutine_allocator;
}

} // namespace kphp::coro::detail::memory
