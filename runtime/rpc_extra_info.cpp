// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime/rpc_extra_info.h"

array<std::pair<rpc_response_extra_info_status_t, rpc_response_extra_info_t>> rpc_responses_extra_info_map;

array<rpc_request_extra_info_t> f$KphpRpcRequestsExtraInfo$$get(class_instance<C$KphpRpcRequestsExtraInfo> v$this) {
  return v$this->extra_info_arr_;
}

Optional<rpc_response_extra_info_t> f$extract_kphp_rpc_response_extra_info(std::int64_t resumable_id) {
  const auto* resp_extra_info_ptr = rpc_responses_extra_info_map.find_value(resumable_id);

  if (resp_extra_info_ptr == nullptr || resp_extra_info_ptr->first == rpc_response_extra_info_status_t::NOT_READY) {
    return {};
  }

  const auto res = resp_extra_info_ptr->second;
  rpc_responses_extra_info_map.unset(resumable_id);
  return res;
}
