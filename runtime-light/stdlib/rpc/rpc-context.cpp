//  Compiler for PHP (aka KPHP)
//  Copyright (c) 2024 LLC «V Kontakte»
//  Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime-light/stdlib/rpc/rpc-context.h"

#include "runtime-light/component/component.h"
#include "runtime-light/component/image.h"

RpcInstanceState &RpcInstanceState::get() noexcept {
  return InstanceState::get().rpc_instance_state;
}

const RpcImageState &RpcImageState::get() noexcept {
  return ImageState::get().rpc_image_state;
}

RpcImageState &RpcImageState::get_mutable() noexcept {
  return ImageState::get_mutable().rpc_image_state;
}
