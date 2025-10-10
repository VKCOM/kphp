// Compiler for PHP (aka KPHP)
// Copyright (c) 2025 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime-light/state/instance-state.h"
#include "runtime-light/stdlib/fork/wait-queue-state.h"

WaitQueueInstanceState& WaitQueueInstanceState::get() noexcept {
  return InstanceState::get().wait_queue_instance_state;
}
