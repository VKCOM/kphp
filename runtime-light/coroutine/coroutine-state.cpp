// Compiler for PHP (aka KPHP)
// Copyright (c) 2025 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime-light/coroutine/coroutine-state.h"

#include "runtime-light/state/instance-state.h"

CoroutineInstanceState& CoroutineInstanceState::get() noexcept {
  return InstanceState::get().coroutine_instance_state;
}

kphp::coro::async_stack_root* CoroutineInstanceState::get_next_root() noexcept {
  return CoroutineInstanceState::get().next_root;
}

void CoroutineInstanceState::set_next_root(kphp::coro::async_stack_root* new_next_root) noexcept {
  CoroutineInstanceState::get().next_root = new_next_root;
}