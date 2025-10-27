// Compiler for PHP (aka KPHP)
// Copyright (c) 2025 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime-light/stdlib/rpc/rpc-queue-state.h"

#include "runtime-light/state/instance-state.h"

RpcQueueInstanceState& RpcQueueInstanceState::get() noexcept {
  return InstanceState::get().rpc_queue_instance_state;
}
