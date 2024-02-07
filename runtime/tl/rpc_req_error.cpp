// Compiler for PHP (aka KPHP)
// Copyright (c) 2023 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime/tl/rpc_req_error.h"

#include "common/rpc-error-codes.h"
#include "common/tl/constants/common.h"
#include "runtime/rpc.h"

bool RpcError::try_fetch() noexcept {
  int pos_backup = rpc_get_pos();
  int op = tl_parse_int();
  if (op == TL_REQ_RESULT_HEADER) {
    int flags = tl_parse_int();
    fetch_and_skip_header(flags);
    op = tl_parse_int();
  }
  if (op != TL_RPC_REQ_ERROR) {
    rpc_set_pos(pos_backup);
    return false;
  }

  tl_parse_long();
  error_code = tl_parse_int();
  error_msg = tl_parse_string();

  if (!CurException.is_null()) {
    error_code = TL_ERROR_SYNTAX;
    error_msg = std::move(CurException->$message);
    CurException = Optional<bool>{};
  }
  return true;
}

void RpcError::fetch_and_skip_header(int flags) const noexcept {
  if (flags & vk::tl::common::rpc_req_result_extra_flags::binlog_pos) {
    tl_parse_long();
  }
  if (flags & vk::tl::common::rpc_req_result_extra_flags::binlog_time) {
    tl_parse_long();
  }
  if (flags & vk::tl::common::rpc_req_result_extra_flags::engine_pid) {
    tl_parse_int();
    tl_parse_int();
    tl_parse_int();
  }
  if (flags & vk::tl::common::rpc_req_result_extra_flags::request_size) {
    tl_parse_int();
  }
  if (flags & vk::tl::common::rpc_req_result_extra_flags::response_size) {
    tl_parse_int();
  }
  if (flags & vk::tl::common::rpc_req_result_extra_flags::failed_subqueries) {
    tl_parse_int();
  }
  if (flags & vk::tl::common::rpc_req_result_extra_flags::compression_version) {
    tl_parse_int();
  }
  if (flags & vk::tl::common::rpc_req_result_extra_flags::stats) {
    size_t n = tl_parse_int();
    for (int i = 0; i < n; ++i) {
      tl_parse_string();
      tl_parse_string();
    }
  }
  if (flags & vk::tl::common::rpc_req_result_extra_flags::epoch_number) {
    tl_parse_long();
  }
  if (flags & vk::tl::common::rpc_req_result_extra_flags::view_number) {
    tl_parse_long();
  }
}
