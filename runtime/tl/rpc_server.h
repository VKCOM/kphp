#pragma once

#include "runtime/tl/tl_func_base.h"

class CurrentRpcServerQuery {
public:
  static CurrentRpcServerQuery &get() {
    static CurrentRpcServerQuery ctx;
    return ctx;
  }

  void save(std::unique_ptr<tl_func_base> tl_func_state);
  std::unique_ptr<tl_func_base> extract();
  void reset();
private:
  std::unique_ptr<tl_func_base> query;
  CurrentRpcServerQuery() = default;
};

struct C$VK$TL$RpcFunction;
struct C$VK$TL$RpcFunctionReturnResult;

class_instance<C$VK$TL$RpcFunction> f$rpc_server_fetch_request();
void f$rpc_server_store_response(const class_instance<C$VK$TL$RpcFunctionReturnResult> &response);