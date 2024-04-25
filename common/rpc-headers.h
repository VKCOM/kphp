// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cstdint>
#include <optional>
#include <utility>

#pragma pack(push, 1)

struct RpcDestActorFlagsHeaders {
  int op{-1};
  long long actor_id{-1};
  int flags{-1};
};

struct RpcDestActorHeaders {
  int op{-1};
  long long actor_id{-1};
};

struct RpcDestFlagsHeaders {
  int op{-1};
  int flags{-1};
};

union RpcExtraHeaders {
  RpcDestActorFlagsHeaders rpc_dest_actor_flags;
  RpcDestActorHeaders rpc_dest_actor;
  RpcDestFlagsHeaders rpc_dest_flags;
};

struct RpcHeaders {
  int length{-1};
  int num{-1};

  int op{-1};  // will be replaced with magic in send_rpc_query()
  long long req_id{-1};

  explicit RpcHeaders(int op)
    : op(op) {}
};

#pragma pack(pop)

// Returns a pair where:
// 1. first element optionally contains a new combinator and its size;
// 2. second element is the size of a current combinator (0 if there is no one).
std::pair<std::optional<std::pair<RpcExtraHeaders, std::size_t>>, std::size_t>
regularize_combinators(const char *rpc_buf, std::int32_t actor_id, bool ignore_result);
