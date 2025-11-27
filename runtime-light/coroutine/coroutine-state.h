// Compiler for PHP (aka KPHP)
// Copyright (c) 2025 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "common/mixin/not_copyable.h"

#include "runtime-light/coroutine/async-stack.h"

struct CoroutineInstanceState final : private vk::not_copyable {

  CoroutineInstanceState() noexcept = default;

  static CoroutineInstanceState& get() noexcept;

  kphp::coro::async_stack_root base_coroutine_stack_root; // used to reduce the number of accesses to thread-local state
  kphp::coro::async_stack_root* current_async_stack_root{&base_coroutine_stack_root};
};
