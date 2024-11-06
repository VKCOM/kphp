//  Compiler for PHP (aka KPHP)
//  Copyright (c) 2024 LLC «V Kontakte»
//  Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime-light/stdlib/rpc/rpc-context.h"

#include "runtime-light/component/component.h"
#include "runtime-light/component/image.h"
#include "runtime-light/utils/context.h"

RpcComponentContext &RpcComponentContext::get() noexcept {
  return get_component_context()->rpc_instance_state;
}

const RpcImageState &RpcImageState::get() noexcept {
  return get_image_state()->rpc_image_state;
}

RpcImageState &RpcImageState::get_mutable() noexcept {
  return get_mutable_image_state()->rpc_image_state;
}
