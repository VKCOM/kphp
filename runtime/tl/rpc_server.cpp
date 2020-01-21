#include "runtime/tl/rpc_server.h"

#include "runtime/rpc.h"

// Определение f$rpc_server_fetch_request() генерируется в файл tl/rpc_server_fetch_request.cpp.
// Представляет из себя:
//    1. fetching magic
//    2. switch по всем @kphp функциям
//    3. сохранение tl_func_state в CurrentRpcServerQuery

void f$rpc_server_store_response(const class_instance<C$VK$TL$RpcFunctionReturnResult> &response) {
  f$rpc_clean();
  std::unique_ptr<tl_func_base> tl_func_state = CurrentRpcServerQuery::get().extract();
  if (!tl_func_state) {
    php_warning("There is no pending rpc server request. Unable to store response.");
    return;
  }
  tl_func_state->rpc_server_typed_store(response);
  f$store_finish();
}

void CurrentRpcServerQuery::save(std::unique_ptr<tl_func_base> tl_func_state) {
  this->query = std::move(tl_func_state);
}

std::unique_ptr<tl_func_base> CurrentRpcServerQuery::extract() {
  return std::move(this->query);
}

void CurrentRpcServerQuery::reset() {
  this->query.reset(nullptr);
}

