// Compiler for PHP (aka KPHP)
// Copyright (c) 2023 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "common/mixin/not_copyable.h"
#include "common/smart_ptrs/singleton.h"
#include "runtime/kphp_core.h"

struct RpcQueriesStat : vk::not_copyable {
public:
  void register_query(int64_t actor_id, int32_t size) noexcept;

  void register_loading_by_script(int64_t actor_id, int32_t size) noexcept;

  const auto &rpc_stats_by_actor() const noexcept {
    return rpc_stats_by_actor_;
  }

  void reset();
private:
  // 0 - queries count, 1 - incoming bytes, 2 - outgoing bytes
  array<std::tuple<int64_t, int64_t, int64_t>> rpc_stats_by_actor_;

  RpcQueriesStat() = default;


  friend class vk::singleton<RpcQueriesStat>;
};
