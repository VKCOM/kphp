// Compiler for PHP (aka KPHP)
// Copyright (c) 2025 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime-light/server/rpc/rpc-server-state.h"

#include "runtime-light/state/instance-state.h"

RpcServerInstanceState& RpcServerInstanceState::get() noexcept {
  return InstanceState::get().rpc_server_instance_state;
}
