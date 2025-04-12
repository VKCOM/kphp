// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime/tl/rpc_response.h"

#include "common/rpc-error-codes.h"
#include "common/tl/constants/common.h"

#include "runtime/exception.h"
#include "runtime/rpc.h"
#include "runtime/tl/rpc_req_error.h"

class_instance<C$VK$TL$RpcResponse> RpcErrorFactory::make_error(const char* error, int error_code) const {
  return make_error(string{error}, error_code);
}

class_instance<C$VK$TL$RpcResponse> RpcErrorFactory::make_error_from_exception_if_possible() const {
  if (!CurException.is_null()) {
    auto rpc_error = make_error(CurException->$message, TL_ERROR_SYNTAX);
    CurException = Optional<bool>{};
    return rpc_error;
  }
  return {};
}

class_instance<C$VK$TL$RpcResponse> RpcErrorFactory::fetch_error_if_possible() const {
  RpcError rpc_error;
  if (!rpc_error.try_fetch()) {
    return {};
  }
  return make_error(rpc_error.error_msg, rpc_error.error_code);
}
