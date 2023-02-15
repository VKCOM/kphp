// Compiler for PHP (aka KPHP)
// Copyright (c) 2023 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime/rpc_queries_stat.h"

void RpcQueriesStat::register_query(int64_t actor_id, int32_t size) noexcept {
  auto &stat = rpc_stats_by_actor_[actor_id];
  std::get<0>(stat) += 1;
  std::get<2>(stat) += size;
}

void RpcQueriesStat::register_loading_by_script(int64_t actor_id, int32_t size) noexcept {
  auto &stat = rpc_stats_by_actor_[actor_id];
  std::get<1>(stat) += size;
}

void RpcQueriesStat::reset() {
  hard_reset_var(rpc_stats_by_actor_);
}
