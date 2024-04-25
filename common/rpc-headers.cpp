// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "common/algorithms/find.h"
#include "common/rpc-headers.h"
#include "common/tl/constants/common.h"

std::pair<std::optional<std::pair<RpcExtraHeaders, std::size_t>>, std::size_t>
regularize_wrappers(const char *rpc_payload, std::int32_t actor_id, bool ignore_result) {
  static_assert(sizeof(RpcDestActorFlagsHeaders) >= sizeof(RpcDestActorHeaders));
  static_assert(sizeof(RpcDestActorFlagsHeaders) >= sizeof(RpcDestFlagsHeaders));

  const auto cur_wrapper{*reinterpret_cast<const RpcExtraHeaders *>(rpc_payload)};
  const auto function_magic{*reinterpret_cast<const uint32_t *>(rpc_payload)};

  if (actor_id == 0 && !ignore_result && vk::none_of_equal(function_magic, TL_RPC_DEST_ACTOR, TL_RPC_DEST_FLAGS, TL_RPC_DEST_ACTOR_FLAGS)) {
    return {std::nullopt, 0};
  }

  RpcExtraHeaders extra_headers{};
  const std::size_t new_wrapper_size{sizeof(RpcDestActorFlagsHeaders)};
  std::size_t cur_wrapper_size{0};

  switch (function_magic) {
    case TL_RPC_DEST_ACTOR_FLAGS:
      cur_wrapper_size = sizeof(RpcDestActorFlagsHeaders);
      extra_headers.rpc_dest_actor_flags.op = TL_RPC_DEST_ACTOR_FLAGS;
      extra_headers.rpc_dest_actor_flags.actor_id = actor_id != 0 ? actor_id : cur_wrapper.rpc_dest_actor_flags.actor_id;
      extra_headers.rpc_dest_actor_flags.flags = cur_wrapper.rpc_dest_actor_flags.flags |
                                                 (ignore_result ? vk::tl::common::rpc_invoke_req_extra_flags::no_result : 0x0);
      break;
    case TL_RPC_DEST_ACTOR:
      cur_wrapper_size = sizeof(RpcDestActorHeaders);
      extra_headers.rpc_dest_actor_flags.op = TL_RPC_DEST_ACTOR_FLAGS;
      extra_headers.rpc_dest_actor_flags.actor_id = actor_id != 0 ? actor_id : cur_wrapper.rpc_dest_actor.actor_id;
      extra_headers.rpc_dest_actor_flags.flags = ignore_result ? vk::tl::common::rpc_invoke_req_extra_flags::no_result : 0x0;
      break;
    case TL_RPC_DEST_FLAGS:
      cur_wrapper_size = sizeof(RpcDestFlagsHeaders);
      extra_headers.rpc_dest_actor_flags.op = TL_RPC_DEST_ACTOR_FLAGS;
      extra_headers.rpc_dest_actor_flags.actor_id = actor_id;
      extra_headers.rpc_dest_actor_flags.flags = cur_wrapper.rpc_dest_flags.flags |
                                                 (ignore_result ? vk::tl::common::rpc_invoke_req_extra_flags::no_result : 0x0);
      break;
    default:
      // we don't have a cur_wrapper, but we do have 'actor_id' or 'ignore_result' set
      extra_headers.rpc_dest_actor_flags.op = TL_RPC_DEST_ACTOR_FLAGS;
      extra_headers.rpc_dest_actor_flags.actor_id = actor_id;
      extra_headers.rpc_dest_actor_flags.flags = ignore_result ? vk::tl::common::rpc_invoke_req_extra_flags::no_result : 0x0;
      break;
  }

  return {std::pair<RpcExtraHeaders, std::size_t>{extra_headers, new_wrapper_size}, cur_wrapper_size};
}
