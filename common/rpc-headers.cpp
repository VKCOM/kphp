// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "common/rpc-headers.h"
#include "common/algorithms/find.h"
#include "common/tl/constants/common.h"


RegularizeWrappersReturnT regularize_wrappers(const char *rpc_payload, std::int32_t actor_id, bool ignore_result) {
  static_assert(sizeof(RpcDestActorFlagsHeaders) >= sizeof(RpcDestActorHeaders));
  static_assert(sizeof(RpcDestActorFlagsHeaders) >= sizeof(RpcDestFlagsHeaders));

  const auto cur_wrapper{*reinterpret_cast<const RpcExtraHeaders *>(rpc_payload)};
  const auto function_magic{*reinterpret_cast<const std::uint32_t *>(rpc_payload)};

  if (actor_id == 0 && !ignore_result && vk::none_of_equal(function_magic, TL_RPC_DEST_ACTOR, TL_RPC_DEST_FLAGS, TL_RPC_DEST_ACTOR_FLAGS)) {
    return {std::nullopt, 0, std::nullopt, nullptr};
  }

  RpcExtraHeaders extra_headers{};
  const std::size_t new_wrapper_size{sizeof(RpcDestActorFlagsHeaders)};
  std::size_t cur_wrapper_size{0};
  std::int32_t cur_wrapper_actor_id{0};
  bool cur_wrapper_ignore_result{false};

  switch (function_magic) {
    case TL_RPC_DEST_ACTOR_FLAGS:
      cur_wrapper_size = sizeof(RpcDestActorFlagsHeaders);
      cur_wrapper_actor_id = cur_wrapper.rpc_dest_actor_flags.actor_id;
      cur_wrapper_ignore_result = static_cast<bool>(cur_wrapper.rpc_dest_actor_flags.flags & vk::tl::common::rpc_invoke_req_extra_flags::no_result);

      extra_headers.rpc_dest_actor_flags.op = TL_RPC_DEST_ACTOR_FLAGS;
      extra_headers.rpc_dest_actor_flags.actor_id = actor_id != 0 ? actor_id : cur_wrapper.rpc_dest_actor_flags.actor_id;
      if (ignore_result) {
        extra_headers.rpc_dest_actor_flags.flags = cur_wrapper.rpc_dest_actor_flags.flags | vk::tl::common::rpc_invoke_req_extra_flags::no_result;
      } else {
        extra_headers.rpc_dest_actor_flags.flags = cur_wrapper.rpc_dest_actor_flags.flags & ~vk::tl::common::rpc_invoke_req_extra_flags::no_result;
      }

      break;
    case TL_RPC_DEST_ACTOR:
      cur_wrapper_size = sizeof(RpcDestActorHeaders);
      cur_wrapper_actor_id = cur_wrapper.rpc_dest_actor.actor_id;

      extra_headers.rpc_dest_actor_flags.op = TL_RPC_DEST_ACTOR_FLAGS;
      extra_headers.rpc_dest_actor_flags.actor_id = actor_id != 0 ? actor_id : cur_wrapper.rpc_dest_actor.actor_id;
      extra_headers.rpc_dest_actor_flags.flags = ignore_result ? vk::tl::common::rpc_invoke_req_extra_flags::no_result : 0x0;

      break;
    case TL_RPC_DEST_FLAGS:
      cur_wrapper_size = sizeof(RpcDestFlagsHeaders);
      cur_wrapper_ignore_result = static_cast<bool>(cur_wrapper.rpc_dest_flags.flags & vk::tl::common::rpc_invoke_req_extra_flags::no_result);

      extra_headers.rpc_dest_actor_flags.op = TL_RPC_DEST_ACTOR_FLAGS;
      extra_headers.rpc_dest_actor_flags.actor_id = actor_id;
      if (ignore_result) {
        extra_headers.rpc_dest_actor_flags.flags = cur_wrapper.rpc_dest_flags.flags | vk::tl::common::rpc_invoke_req_extra_flags::no_result;
      } else {
        extra_headers.rpc_dest_actor_flags.flags = cur_wrapper.rpc_dest_flags.flags & ~vk::tl::common::rpc_invoke_req_extra_flags::no_result;
      }

      break;
    default:
      // we don't have a cur_wrapper, but we do have 'actor_id' or 'ignore_result' set
      extra_headers.rpc_dest_actor_flags.op = TL_RPC_DEST_ACTOR_FLAGS;
      extra_headers.rpc_dest_actor_flags.actor_id = actor_id;
      extra_headers.rpc_dest_actor_flags.flags = ignore_result ? vk::tl::common::rpc_invoke_req_extra_flags::no_result : 0x0;

      break;
  }

  decltype(RegularizeWrappersReturnT{}.opt_actor_id_warning_info) opt_actor_id_warning{};
  if (actor_id != 0 && cur_wrapper_actor_id != 0) {
    opt_actor_id_warning.emplace("inaccurate use of 'actor_id': '%d' was passed into RPC connection constructor, "
                                 "but '%d' was already set in RpcDestActor or RpcDestActorFlags\n",
                                 actor_id, cur_wrapper_actor_id);
  }

  const char *opt_ignore_result_warning_msg{nullptr};
  if (!ignore_result && cur_wrapper_ignore_result) {
    opt_ignore_result_warning_msg = "inaccurate use of 'ignore_answer': 'false' was passed into TL query function (e.g., rpc_tl_query), "
                                    "but 'true' was already set in RpcDestFlags or RpcDestActorFlags\n";
  }

  return {
    std::pair<RpcExtraHeaders, std::size_t>{extra_headers, new_wrapper_size},
    cur_wrapper_size,
    std::move(opt_actor_id_warning),
    opt_ignore_result_warning_msg,
  };
}
