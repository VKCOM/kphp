// Compiler for PHP (aka KPHP)
// Copyright (c) 2025 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "common/mixin/not_copyable.h"

#include "runtime-light/coroutine/async-stack.h"

struct CoroutineInstanceState final : private vk::not_copyable {

  CoroutineInstanceState() = default;

  static CoroutineInstanceState& get() noexcept;

  kphp::coro::async_stack_root& get_coroutine_stack_root() noexcept {
    return coroutine_stack_root;
  }

  kphp::coro::async_stack_root coroutine_stack_root;
};
