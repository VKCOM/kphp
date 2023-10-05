// Compiler for PHP (aka KPHP)
// Copyright (c) 2023 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "vkext/vkext-rpc-req-error.h"

#include "common/tl/constants/common.h"

namespace vkext_rpc {
void tl::NetPid::tl_fetch() {
  ip = tl_parse_int();
  port_pid = tl_parse_int();
  utime = tl_parse_int();
}

void tl::RpcReqResultExtra::tl_fetch(int flags) {
  if (flags & vk::tl::common::rpc_req_result_extra_flags::binlog_pos) {
    binlog_pos = tl_parse_long();
  }
  if (flags & vk::tl::common::rpc_req_result_extra_flags::binlog_time) {
    binlog_time = tl_parse_long();
  }
  if (flags & vk::tl::common::rpc_req_result_extra_flags::engine_pid) {
    engine_pid.emplace().tl_fetch();
  }
  if (flags & vk::tl::common::rpc_req_result_extra_flags::request_size) {
    request_size = tl_parse_int();
  }
  if (flags & vk::tl::common::rpc_req_result_extra_flags::response_size) {
    response_size = tl_parse_int();
  }
  if (flags & vk::tl::common::rpc_req_result_extra_flags::failed_subqueries) {
    failed_subqueries = tl_parse_int();
  }
  if (flags & vk::tl::common::rpc_req_result_extra_flags::compression_version) {
    compression_version = tl_parse_int();
  }
  if (flags & vk::tl::common::rpc_req_result_extra_flags::stats) {
    size_t n = tl_parse_int();
    stats.emplace();
    for (int i = 0; i < n; ++i) {
      auto key = tl_parse_string();
      auto val = tl_parse_string();
      stats->emplace(std::move(key), std::move(val));
    }
  }
  if (flags & vk::tl::common::rpc_req_result_extra_flags::epoch_number) {
    epoch_number = tl_parse_long();
  }
  if (flags & vk::tl::common::rpc_req_result_extra_flags::view_number) {
    view_number = tl_parse_long();
  }
}

void tl::RpcReqError::tl_fetch() {
  query_id = tl_parse_long();
  error_code = tl_parse_int();
  error_msg = tl_parse_string();
}

void RpcError::try_fetch() {
  int op = tl_parse_int();
  if (op == TL_REQ_RESULT_HEADER) {
    flags = tl_parse_int();
    header.emplace().tl_fetch(flags);
    op = tl_parse_int();
  }
  if (op == TL_RPC_REQ_ERROR) {
    error.emplace().tl_fetch();
  }
}
} // namespace vkext_rpc
