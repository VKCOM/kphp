//  Compiler for PHP (aka KPHP)
//  Copyright (c) 2024 LLC «V Kontakte»
//  Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime-light/stdlib/rpc/rpc-extra-headers.h"

#include <cstddef>
#include <cstdint>
#include <span>

#include "common/algorithms/find.h"
#include "common/tl/constants/common.h"
#include "runtime-light/utils/logs.h"

namespace {

constexpr int64_t EXPECTED_ACTOR_ID = 0;
constexpr uint32_t EMPTY_FLAGS = 0x0;

} // namespace

namespace kphp::rpc {

std::pair<std::optional<dest_actor_flags_header>, uint32_t> regularize_extra_headers(std::span<const std::byte> payload, bool ignore_result) noexcept {
  kphp::log::assertion(payload.size() >= sizeof(uint32_t));
  const auto magic{*reinterpret_cast<const uint32_t*>(payload.data())};
  if (vk::none_of_equal(magic, TL_RPC_DEST_ACTOR, TL_RPC_DEST_FLAGS, TL_RPC_DEST_ACTOR_FLAGS)) [[unlikely]] {
    return {std::nullopt, 0};
  }

  uint32_t cur_extra_header_size{0};
  uint32_t cur_extra_header_flags{EMPTY_FLAGS};
  int64_t cur_extra_header_actor_id{EXPECTED_ACTOR_ID};
  switch (magic) {
  case TL_RPC_DEST_ACTOR_FLAGS: {
    kphp::log::assertion(payload.size() >= sizeof(uint32_t) + sizeof(dest_actor_flags_header));
    cur_extra_header_size = sizeof(dest_actor_flags_header);
    const auto cur_wrapper{*reinterpret_cast<const dest_actor_flags_header*>(payload.data())};
    cur_extra_header_flags = cur_wrapper.flags;
    cur_extra_header_actor_id = cur_wrapper.actor_id;
    break;
  }
  case TL_RPC_DEST_ACTOR: {
    kphp::log::assertion(payload.size() >= sizeof(uint32_t) + sizeof(dest_actor_header));
    cur_extra_header_size = sizeof(dest_actor_header);
    const auto cur_wrapper{*reinterpret_cast<const dest_actor_header*>(payload.data())};
    cur_extra_header_actor_id = cur_wrapper.actor_id;
    break;
  }
  case TL_RPC_DEST_FLAGS: {
    kphp::log::assertion(payload.size() >= sizeof(uint32_t) + sizeof(dest_flags_header));
    cur_extra_header_size = sizeof(dest_flags_header);
    const auto cur_wrapper{*reinterpret_cast<const dest_flags_header*>(payload.data())};
    cur_extra_header_flags = cur_wrapper.flags;
    break;
  }
  default:
    kphp::log::fatal("unreachable");
  }

  if (cur_extra_header_actor_id != EXPECTED_ACTOR_ID) [[unlikely]] {
    kphp::log::warning("RPC extra headers have actor_id set to {}, but it should not be explicitly set", cur_extra_header_actor_id);
  }

  const auto cur_extra_header_ignore_result{static_cast<bool>(cur_extra_header_flags & vk::tl::common::rpc_invoke_req_extra_flags::no_result)};
  if (!ignore_result && cur_extra_header_ignore_result) [[unlikely]] {
    kphp::log::warning("inaccurate use of 'ignore_answer': 'false' was passed into TL query function (e.g., rpc_tl_query), "
                       "but 'true' was already set in RpcDestFlags or RpcDestActorFlags\n");
  }

  return {dest_actor_flags_header{.op = TL_RPC_DEST_ACTOR_FLAGS,
                                  .actor_id = EXPECTED_ACTOR_ID,
                                  .flags = cur_extra_header_flags & ~vk::tl::common::rpc_invoke_req_extra_flags::no_result},
          cur_extra_header_size};
}

} // namespace kphp::rpc
