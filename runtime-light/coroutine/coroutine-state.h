// Compiler for PHP (aka KPHP)
// Copyright (c) 2025 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "common/mixin/not_copyable.h"

#include "runtime-light/allocator/runtime-coroutine-allocator.h"
#include "runtime-light/coroutine/async-stack.h"

namespace kphp::coro {

struct instance_state final : private vk::not_copyable {

  instance_state() noexcept = default;

  static instance_state& get() noexcept;

  kphp::coro::async_stack_root coroutine_stack_root;
  RuntimeCoroutineAllocator coroutine_allocator;
};

} // namespace kphp::coro
