//  Compiler for PHP (aka KPHP)
//  Copyright (c) 2024 LLC «V Kontakte»
//  Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cstdint>
#include <optional>
#include <utility>

#pragma pack(push, 1)

struct RpcDestActorFlagsHeaders {
  uint32_t op;
  int64_t actor_id;
  uint32_t flags;
};

struct RpcDestActorHeaders {
  uint32_t op;
  int64_t actor_id;
};

struct RpcDestFlagsHeaders {
  uint32_t op;
  uint32_t flags;
};

#pragma pack(pop)

/// Check RPC payload whether it contains some extra header. If so:
/// 1) check if actor_id is set in the header; warn if it's set and not equal to 0;
/// 2) check if ignore_result is set in the header; warn if it's set in the header and not set in [typed_]rpc_tl_query call;
/// 3) return <RpcDestActorFlagsHeaders instance, current extra header's size> pair.
/// Otherwise, return <std::nullopt, current extra header's size>.
std::pair<std::optional<RpcDestActorFlagsHeaders>, uint32_t> regularize_extra_headers(const char *rpc_payload, bool ignore_result) noexcept;
