// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "common/algorithms/find.h"
#include "common/rpc-headers.h"
#include "common/tl/constants/common.h"

size_t fill_extra_headers_if_needed(RpcExtraHeaders &extra_headers, uint32_t function_magic, int actor_id, bool ignore_answer) {
  size_t extra_headers_size = 0;
  bool need_actor = actor_id != 0 && vk::none_of_equal(function_magic, TL_RPC_DEST_ACTOR, TL_RPC_DEST_ACTOR_FLAGS);
  bool need_flags = ignore_answer && vk::none_of_equal(function_magic, TL_RPC_DEST_FLAGS, TL_RPC_DEST_ACTOR_FLAGS);

  if (need_actor && need_flags) {
    extra_headers.rpc_dest_actor_flags.op = TL_RPC_DEST_ACTOR_FLAGS;
    extra_headers.rpc_dest_actor_flags.actor_id = actor_id;
    extra_headers.rpc_dest_actor_flags.flags = vk::tl::common::rpc_invoke_req_extra_flags::no_result;
    extra_headers_size = sizeof(extra_headers.rpc_dest_actor_flags);
  } else if (need_actor) {
    extra_headers.rpc_dest_actor.op = TL_RPC_DEST_ACTOR;
    extra_headers.rpc_dest_actor.actor_id = actor_id;
    extra_headers_size = sizeof(extra_headers.rpc_dest_actor);
  } else if (need_flags) {
    extra_headers.rpc_dest_flags.op = TL_RPC_DEST_FLAGS;
    extra_headers.rpc_dest_flags.flags = vk::tl::common::rpc_invoke_req_extra_flags::no_result;
    extra_headers_size = sizeof(extra_headers.rpc_dest_flags);
  }
  return extra_headers_size;
}
