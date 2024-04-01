// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cstddef>
#include <cstdint>

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

size_t fill_extra_headers_if_needed(RpcExtraHeaders &extra_headers, uint32_t function_magic, int actor_id, bool ignore_answer);
