//  Compiler for PHP (aka KPHP)
//  Copyright (c) 2024 LLC «V Kontakte»
//  Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime-light/stdlib/rpc/rpc-tl-error.h"

#include <variant>

#include "runtime-light/server/rpc/rpc-server-state.h"
#include "runtime-light/tl/tl-core.h"
#include "runtime-light/tl/tl-types.h"

bool TlRpcError::try_fetch() noexcept {
  auto& rpc_server_instance_state_fetcher{RpcServerInstanceState::get().tl_fetcher};
  auto fetcher{rpc_server_instance_state_fetcher};
  const auto backup_pos{fetcher.pos()};
  if (tl::ReqResult req_result{}; req_result.fetch(fetcher) && std::holds_alternative<tl::reqResultHeader>(req_result.value)) {
    fetcher = tl::fetcher{std::get<tl::reqResultHeader>(req_result.value).result};
  } else {
    fetcher.reset(backup_pos);
  }
  tl::RpcReqResult rpc_req_result{};
  if (!rpc_req_result.fetch(fetcher) || !std::holds_alternative<tl::rpcReqError>(rpc_req_result.value)) {
    return false;
  }
  auto& rpc_req_error{std::get<tl::rpcReqError>(rpc_req_result.value)};
  error_code = rpc_req_error.error_code.value;
  error_msg = {rpc_req_error.error.value.data(), static_cast<string::size_type>(rpc_req_error.error.value.size())};
  rpc_server_instance_state_fetcher = std::move(fetcher);
  return true;
}
