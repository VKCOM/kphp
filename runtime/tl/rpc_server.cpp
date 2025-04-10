// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime/tl/rpc_server.h"

#include "runtime/rpc.h"

// f$rpc_server_fetch_request() definition is generated into the tl/rpc_server_fetch_request.cpp file.
// It's composed of:
//    1. fetching magic
//    2. switch over all @kphp functions
//    3. tl_func_state storing inside the CurrentRpcServerQuery

void f$rpc_server_store_response(const class_instance<C$VK$TL$RpcFunctionReturnResult>& response) noexcept {
  f$rpc_clean();
  std::unique_ptr<tl_func_base> tl_func_state = CurrentRpcServerQuery::get().extract();
  if (!tl_func_state) {
    php_warning("There is no pending rpc server request. Unable to store response.");
    return;
  }
  tl_func_state->rpc_server_typed_store(response);
  f$store_finish();
}

void CurrentRpcServerQuery::save(std::unique_ptr<tl_func_base> tl_func_state) noexcept {
  this->query = std::move(tl_func_state);
}

std::unique_ptr<tl_func_base> CurrentRpcServerQuery::extract() noexcept {
  return std::move(this->query);
}

void CurrentRpcServerQuery::reset() noexcept {
  this->query.reset(nullptr);
}
