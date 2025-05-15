// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime-light/coroutine/coroutine-state.h"

#include "runtime-light/state/instance-state.h"

CoroutineInstanceState& CoroutineInstanceState::get() noexcept {
  return InstanceState::get().coroutine_instance_state;
}
