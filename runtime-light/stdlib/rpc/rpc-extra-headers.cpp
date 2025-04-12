//  Compiler for PHP (aka KPHP)
//  Copyright (c) 2024 LLC «V Kontakte»
//  Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime-light/stdlib/rpc/rpc-extra-headers.h"

#include <cinttypes>

#include "common/algorithms/find.h"
#include "common/tl/constants/common.h"
#include "runtime-common/core/utils/kphp-assert-core.h"

namespace {

constexpr int64_t EXPECTED_ACTOR_ID = 0;
constexpr uint32_t EMPTY_FLAGS = 0x0;

} // namespace

std::pair<std::optional<RpcDestActorFlagsHeaders>, uint32_t> regularize_extra_headers(const char* rpc_payload, bool ignore_result) noexcept {
  const auto magic{*reinterpret_cast<const uint32_t*>(rpc_payload)};
  if (vk::none_of_equal(magic, TL_RPC_DEST_ACTOR, TL_RPC_DEST_FLAGS, TL_RPC_DEST_ACTOR_FLAGS)) {
    return {std::nullopt, 0};
  }

  uint32_t cur_extra_header_size{0};
  uint32_t cur_extra_header_flags{EMPTY_FLAGS};
  int64_t cur_extra_header_actor_id{EXPECTED_ACTOR_ID};
  switch (magic) {
  case TL_RPC_DEST_ACTOR_FLAGS: {
    cur_extra_header_size = sizeof(RpcDestActorFlagsHeaders);
    const auto cur_wrapper{*reinterpret_cast<const RpcDestActorFlagsHeaders*>(rpc_payload)};
    cur_extra_header_flags = cur_wrapper.flags;
    cur_extra_header_actor_id = cur_wrapper.actor_id;
    break;
  }
  case TL_RPC_DEST_ACTOR: {
    cur_extra_header_size = sizeof(RpcDestActorHeaders);
    const auto cur_wrapper{*reinterpret_cast<const RpcDestActorHeaders*>(rpc_payload)};
    cur_extra_header_actor_id = cur_wrapper.actor_id;
    break;
  }
  case TL_RPC_DEST_FLAGS: {
    cur_extra_header_size = sizeof(RpcDestFlagsHeaders);
    const auto cur_wrapper{*reinterpret_cast<const RpcDestFlagsHeaders*>(rpc_payload)};
    cur_extra_header_flags = cur_wrapper.flags;
    break;
  }
  default: {
    php_critical_error("unreachable path");
  }
  }

  if (cur_extra_header_actor_id != EXPECTED_ACTOR_ID) {
    php_warning("RPC extra headers have actor_id set to %" PRId64 ", but it should not be explicitly set", cur_extra_header_actor_id);
  }
  const auto cur_extra_header_ignore_result{static_cast<bool>(cur_extra_header_flags & vk::tl::common::rpc_invoke_req_extra_flags::no_result)};
  if (!ignore_result && cur_extra_header_ignore_result) {
    php_warning("inaccurate use of 'ignore_answer': 'false' was passed into TL query function (e.g., rpc_tl_query), "
                "but 'true' was already set in RpcDestFlags or RpcDestActorFlags\n");
  }

  return {RpcDestActorFlagsHeaders{.op = TL_RPC_DEST_ACTOR_FLAGS,
                                   .actor_id = EXPECTED_ACTOR_ID,
                                   .flags = cur_extra_header_flags & ~vk::tl::common::rpc_invoke_req_extra_flags::no_result},
          cur_extra_header_size};
}
