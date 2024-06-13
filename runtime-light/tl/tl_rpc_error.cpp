//  Compiler for PHP (aka KPHP)
//  Copyright (c) 2024 LLC «V Kontakte»
//  Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime-light/tl/tl_rpc_error.h"

#include <tuple>

#include "common/tl/constants/common.h"
#include "runtime-light/stdlib/rpc/rpc_api.h"
#include "runtime-light/tl/tl_builtins.h"

bool TlRpcError::try_fetch() noexcept {
  const auto backup_pos{tl_parse_save_pos()};
  auto op{f$fetch_int()};
  if (op == TL_REQ_RESULT_HEADER) {
    fetch_and_skip_header();
    op = f$fetch_int();
  }
  if (op != TL_RPC_REQ_ERROR) {
    tl_parse_restore_pos(backup_pos);
    return false;
  }

  std::ignore = f$fetch_long();
  error_code = static_cast<int32_t>(f$fetch_int());
  error_msg = f$fetch_string();

  // TODO: exception handling
  return true;
}

void TlRpcError::fetch_and_skip_header() const noexcept {
  const auto flags{static_cast<int32_t>(f$fetch_int())};

  if (flags & vk::tl::common::rpc_req_result_extra_flags::binlog_pos) {
    std::ignore = f$fetch_long();
  }
  if (flags & vk::tl::common::rpc_req_result_extra_flags::binlog_time) {
    std::ignore = f$fetch_long();
  }
  if (flags & vk::tl::common::rpc_req_result_extra_flags::engine_pid) {
    std::ignore = f$fetch_int();
    std::ignore = f$fetch_int();
    std::ignore = f$fetch_int();
  }
  if (flags & vk::tl::common::rpc_req_result_extra_flags::request_size) {
    std::ignore = f$fetch_int();
  }
  if (flags & vk::tl::common::rpc_req_result_extra_flags::response_size) {
    std::ignore = f$fetch_int();
  }
  if (flags & vk::tl::common::rpc_req_result_extra_flags::failed_subqueries) {
    std::ignore = f$fetch_int();
  }
  if (flags & vk::tl::common::rpc_req_result_extra_flags::compression_version) {
    std::ignore = f$fetch_int();
  }
  if (flags & vk::tl::common::rpc_req_result_extra_flags::stats) {
    const auto size{f$fetch_int()};
    for (auto i = 0; i < size; ++i) {
      std::ignore = f$fetch_int();
      std::ignore = f$fetch_int();
    }
  }
  if (flags & vk::tl::common::rpc_req_result_extra_flags::epoch_number) {
    std::ignore = f$fetch_long();
  }
  if (flags & vk::tl::common::rpc_req_result_extra_flags::view_number) {
    std::ignore = f$fetch_long();
  }
}

class_instance<C$VK$TL$RpcResponse> RpcErrorFactory::make_error(const char *error, int32_t error_code) const noexcept {
  return make_error(string{error}, error_code);
}

class_instance<C$VK$TL$RpcResponse> RpcErrorFactory::make_error_from_exception_if_possible() const noexcept {
  // TODO
  //  if (!CurException.is_null()) {
  //    auto rpc_error = make_error(CurException->$message, TL_ERROR_SYNTAX);
  //    CurException = Optional<bool>{};
  //    return rpc_error;
  //  }
  return {};
}

class_instance<C$VK$TL$RpcResponse> RpcErrorFactory::fetch_error_if_possible() const noexcept {
  TlRpcError rpc_error{};
  if (!rpc_error.try_fetch()) {
    return {};
  }
  return make_error(rpc_error.error_msg, rpc_error.error_code);
}
