//  Compiler for PHP (aka KPHP)
//  Copyright (c) 2024 LLC «V Kontakte»
//  Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime-light/stdlib/rpc/rpc-tl-error.h"

#include "common/tl/constants/common.h"
#include "runtime-light/server/rpc/rpc-server-state.h"
#include "runtime-light/stdlib/diagnostics/exception-functions.h"
#include "runtime-light/stdlib/rpc/rpc-exceptions.h"
#include "runtime-light/tl/tl-core.h"
#include "runtime-light/tl/tl-types.h"

bool TlRpcError::try_fetch() noexcept {
  auto& rpc_server_instance_state_fetcher{RpcServerInstanceState::get().tl_fetcher};
  auto fetcher{rpc_server_instance_state_fetcher};
  tl::magic magic{};
  if (!magic.fetch(fetcher)) [[unlikely]] {
    THROW_EXCEPTION(kphp::rpc::exception::not_enough_data_to_fetch::make());
    return false;
  }
  if (magic.expect(TL_REQ_RESULT_HEADER)) {
    tl::reqResultHeader req_result_header{};
    if (!req_result_header.fetch(fetcher)) [[unlikely]] {
      THROW_EXCEPTION(kphp::rpc::exception::cant_fetch_header::make());
      return false;
    }
    fetcher = tl::fetcher{req_result_header.result};
  }
  if (!magic.expect(TL_RPC_REQ_ERROR)) {
    return false;
  }
  tl::rpcReqError rpc_req_error{};
  if (!rpc_req_error.fetch(fetcher)) [[unlikely]] {
    THROW_EXCEPTION(kphp::rpc::exception::cant_fetch_error::make());
    return false;
  }
  error_code = rpc_req_error.error_code.value;
  error_msg = {rpc_req_error.error.value.data(), static_cast<string::size_type>(rpc_req_error.error.value.size())};
  rpc_server_instance_state_fetcher = std::move(fetcher);
  return true;
}
