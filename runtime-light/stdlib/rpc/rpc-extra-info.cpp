// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime-light/stdlib/rpc/rpc-extra-info.h"

#include <cstdint>
#include <tuple>

#include "runtime-light/stdlib/rpc/rpc-state.h"

Optional<rpc_response_extra_info_t> f$extract_kphp_rpc_response_extra_info(int64_t query_id) noexcept {
  auto &extra_info_map{RpcInstanceState::get().rpc_responses_extra_info};
  if (const auto it{extra_info_map.find(query_id)}; it != extra_info_map.end() && it->second.first == rpc_response_extra_info_status_t::READY) {
    const auto extra_info{it->second.second};
    extra_info_map.erase(it);
    return extra_info;
  }
  return {};
}
