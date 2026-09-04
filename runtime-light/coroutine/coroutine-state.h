// Compiler for PHP (aka KPHP)
// Copyright (c) 2025 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cstddef>

#include "common/mixin/not_copyable.h"

#include "runtime-common/core/allocator/pool-allocator.h"
#include "runtime-light/coroutine/async-stack.h"

namespace kphp::coro {

struct instance_state final : private vk::not_copyable {

  instance_state(size_t coroutine_mem_size, size_t min_extra_coroutine_mem_size, size_t oom_handling_coroutine_mem_size) noexcept
      : coroutine_allocator{coroutine_mem_size, min_extra_coroutine_mem_size, oom_handling_coroutine_mem_size} {}

  static instance_state& get() noexcept;

  static kphp::memory::pool_allocator& get_coroutine_allocator() noexcept {
    return kphp::coro::instance_state::get().coroutine_allocator;
  }

  kphp::memory::pool_allocator coroutine_allocator;
  kphp::coro::async_stack_root coroutine_stack_root;
};

} // namespace kphp::coro
