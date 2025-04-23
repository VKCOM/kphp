//  Compiler for PHP (aka KPHP)
//  Copyright (c) 2024 LLC «V Kontakte»
//  Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime-light/stdlib/rpc/rpc-state.h"

#include "runtime-light/state/image-state.h"
#include "runtime-light/state/instance-state.h"

RpcClientInstanceState& RpcClientInstanceState::get() noexcept {
  return InstanceState::get().rpc_client_instance_state;
}

const RpcImageState& RpcImageState::get() noexcept {
  return ImageState::get().rpc_image_state;
}

RpcImageState& RpcImageState::get_mutable() noexcept {
  return ImageState::get_mutable().rpc_image_state;
}
