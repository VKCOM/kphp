// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "runtime/tl/tl_func_base.h"

class CurrentRpcServerQuery {
public:
  static CurrentRpcServerQuery &get() noexcept {
    static CurrentRpcServerQuery ctx;
    return ctx;
  }

  void save(std::unique_ptr<tl_func_base> tl_func_state) noexcept;
  std::unique_ptr<tl_func_base> extract() noexcept;
  void reset() noexcept;

private:
  std::unique_ptr<tl_func_base> query;
  CurrentRpcServerQuery() = default;
};

struct C$VK$TL$RpcFunction;
struct C$VK$TL$RpcFunctionReturnResult;

class_instance<C$VK$TL$RpcFunction> f$rpc_server_fetch_request() noexcept;
void f$rpc_server_store_response(const class_instance<C$VK$TL$RpcFunctionReturnResult> &response) noexcept;
