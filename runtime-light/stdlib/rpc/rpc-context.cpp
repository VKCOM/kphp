//  Compiler for PHP (aka KPHP)
//  Copyright (c) 2024 LLC «V Kontakte»
//  Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime-light/stdlib/rpc/rpc-context.h"

#include "runtime-light/component/component.h"
#include "runtime-light/component/image.h"

RpcComponentContext::RpcComponentContext(memory_resource::unsynchronized_pool_resource &memory_resource)
  : current_query()
  , pending_component_queries(unordered_map<int64_t, class_instance<C$ComponentQuery>>::allocator_type{memory_resource})
  , pending_rpc_queries(unordered_map<int64_t, class_instance<RpcTlQuery>>::allocator_type{memory_resource})
  , rpc_responses_extra_info(unordered_map<int64_t, std::pair<rpc_response_extra_info_status_t, rpc_response_extra_info_t>>::allocator_type{memory_resource}) {}

RpcComponentContext &RpcComponentContext::current() noexcept {
  return get_component_context()->rpc_component_context;
}

const RpcImageState &RpcImageState::current() noexcept {
  return get_image_state()->rpc_image_state;
}

RpcImageState &RpcImageState::current_mutable() noexcept {
  return get_mutable_image_state()->rpc_image_state;
}
