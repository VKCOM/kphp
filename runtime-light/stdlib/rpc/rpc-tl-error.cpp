//  Compiler for PHP (aka KPHP)
//  Copyright (c) 2024 LLC «V Kontakte»
//  Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime-light/stdlib/rpc/rpc-tl-error.h"

#include "common/tl/constants/common.h"
#include "runtime-light/server/rpc/rpc-server-state.h"
#include "runtime-light/stdlib/diagnostics/exception-functions.h"
#include "runtime-light/stdlib/rpc/rpc-exceptions.h"
#include "runtime-light/tl/tl-types.h"

bool TlRpcError::try_fetch() noexcept {
  auto& fetcher{RpcServerInstanceState::get().tl_fetcher};
  const auto backup_pos{fetcher.pos()};
  tl::magic tl_op;
  if (!tl_op.fetch(fetcher)) [[unlikely]] {
    THROW_EXCEPTION(kphp::rpc::exception::not_enough_data_to_fetch::make());
    return false;
  }
  const auto& op{tl_op.value};
  if (op == TL_REQ_RESULT_HEADER) {
    tl::mask extra_flags{};
    if (!extra_flags.fetch(fetcher)) [[unlikely]] {
      THROW_EXCEPTION(kphp::rpc::exception::not_enough_data_to_fetch::make());
      return false;
    }
    if (!tl::rpcReqResultExtra{}.fetch(fetcher, extra_flags)) [[unlikely]] {
      THROW_EXCEPTION(kphp::rpc::exception::cant_fetch_header::make());
      return false;
    }
    if (!tl_op.fetch(fetcher)) [[unlikely]] {
      THROW_EXCEPTION(kphp::rpc::exception::not_enough_data_to_fetch::make());
      return false;
    }
  }
  if (op != TL_RPC_REQ_ERROR) {
    fetcher.reset(backup_pos);
    return false;
  }
  tl::rpcReqError rpc_req_error;
  if (!rpc_req_error.fetch(fetcher)) [[unlikely]] {
    THROW_EXCEPTION(kphp::rpc::exception::cant_fetch_error::make());
    return false;
  }
  error_code = rpc_req_error.error_code.value;
  error_msg = {rpc_req_error.error.value.data(), static_cast<string::size_type>(rpc_req_error.error.value.size())};
  return true;
}
