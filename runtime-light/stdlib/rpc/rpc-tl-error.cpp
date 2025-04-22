//  Compiler for PHP (aka KPHP)
//  Copyright (c) 2024 LLC «V Kontakte»
//  Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime-light/stdlib/rpc/rpc-tl-error.h"

#include <tuple>

#include "common/tl/constants/common.h"
#include "runtime-light/stdlib/diagnostics/exception-functions.h"
#include "runtime-light/stdlib/rpc/rpc-api.h"
#include "runtime-light/stdlib/rpc/rpc-tl-builtins.h"

bool TlRpcError::try_fetch() noexcept {
  const auto backup_pos{tl_parse_save_pos()};
  auto op{TRY_CALL(decltype(f$fetch_int()), bool, f$fetch_int())};
  if (op == TL_REQ_RESULT_HEADER) {
    fetch_and_skip_header();
    op = TRY_CALL(decltype(f$fetch_int()), bool, f$fetch_int());
  }
  if (op != TL_RPC_REQ_ERROR) {
    tl_parse_restore_pos(backup_pos);
    return false;
  }

  std::ignore = TRY_CALL(decltype(f$fetch_long()), bool, f$fetch_long());
  error_code = static_cast<int32_t>(TRY_CALL(decltype(f$fetch_int()), bool, f$fetch_int()));
  error_msg = TRY_CALL(decltype(f$fetch_string()), bool, f$fetch_string());
  return true;
}

void TlRpcError::fetch_and_skip_header() const noexcept {
  const auto flags{static_cast<int32_t>(TRY_CALL(decltype(f$fetch_int()), void, f$fetch_int()))};

  if (flags & vk::tl::common::rpc_req_result_extra_flags::binlog_pos) {
    std::ignore = TRY_CALL(decltype(f$fetch_long()), void, f$fetch_long());
  }
  if (flags & vk::tl::common::rpc_req_result_extra_flags::binlog_time) {
    std::ignore = TRY_CALL(decltype(f$fetch_long()), void, f$fetch_long());
  }
  if (flags & vk::tl::common::rpc_req_result_extra_flags::engine_pid) {
    std::ignore = TRY_CALL(decltype(f$fetch_int()), void, f$fetch_int());
    std::ignore = TRY_CALL(decltype(f$fetch_int()), void, f$fetch_int());
    std::ignore = TRY_CALL(decltype(f$fetch_int()), void, f$fetch_int());
  }
  if (flags & vk::tl::common::rpc_req_result_extra_flags::request_size) {
    std::ignore = TRY_CALL(decltype(f$fetch_int()), void, f$fetch_int());
  }
  if (flags & vk::tl::common::rpc_req_result_extra_flags::response_size) {
    std::ignore = TRY_CALL(decltype(f$fetch_int()), void, f$fetch_int());
  }
  if (flags & vk::tl::common::rpc_req_result_extra_flags::failed_subqueries) {
    std::ignore = TRY_CALL(decltype(f$fetch_int()), void, f$fetch_int());
  }
  if (flags & vk::tl::common::rpc_req_result_extra_flags::compression_version) {
    std::ignore = TRY_CALL(decltype(f$fetch_int()), void, f$fetch_int());
  }
  if (flags & vk::tl::common::rpc_req_result_extra_flags::stats) {
    const auto size{TRY_CALL(decltype(f$fetch_int()), void, f$fetch_int())};
    for (auto i = 0; i < size; ++i) {
      std::ignore = TRY_CALL(decltype(f$fetch_int()), void, f$fetch_int());
      std::ignore = TRY_CALL(decltype(f$fetch_int()), void, f$fetch_int());
    }
  }
  if (flags & vk::tl::common::rpc_req_result_extra_flags::epoch_number) {
    std::ignore = TRY_CALL(decltype(f$fetch_long()), void, f$fetch_long());
  }
  if (flags & vk::tl::common::rpc_req_result_extra_flags::view_number) {
    std::ignore = TRY_CALL(decltype(f$fetch_long()), void, f$fetch_long());
  }
}
