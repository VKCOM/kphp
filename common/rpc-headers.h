// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cstdint>
#include <optional>
#include <tuple>
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

struct RegularizeWrappersReturnT {
  /// Optionally contains a new wrapper and its size
  std::optional<std::pair<RpcExtraHeaders, size_t>> opt_new_wrapper;
  /// The size of a wrapper found in rpc payload (0 if there is no one)
  size_t cur_wrapper_size{0};
  /// Optionally contains a tuple of <format string, current wrapper's actor_id, new actor_id>.
  /// If not std::nullopt, can be used to warn about actor_id redefinition, for example,
  /// 'php_warning(format_str, current_wrapper_actor_id, new_actor_id)'
  std::optional<std::tuple<const char *, int32_t, int32_t>> opt_actor_id_warning_info;
  /// Optionally contains a string. If not nullptr, can be used to warn about inaccurate usage of 'ignore_result'.
  const char *opt_ignore_result_warning_msg{nullptr};
};

RegularizeWrappersReturnT regularize_wrappers(const char *rpc_payload, int32_t actor_id, bool ignore_result);
