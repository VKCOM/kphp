#include "runtime/tl/rpc_response.h"

#include "common/rpc-error-codes.h"
#include "common/tl/constants/common.h"

#include "runtime/exception.h"
#include "runtime/rpc.h"

class_instance<C$VK$TL$RpcResponse> RpcErrorFactory::make_error(const char *error, int error_code) const {
  return make_error(string{error}, error_code);
}

class_instance<C$VK$TL$RpcResponse> RpcErrorFactory::make_error_from_exception_if_possible() const {
  if (!CurException.is_null()) {
    auto rpc_error = make_error(CurException->message, TL_ERROR_SYNTAX);
    CurException = Optional<bool>{};
    return rpc_error;
  }
  return {};
}

class_instance<C$VK$TL$RpcResponse> RpcErrorFactory::fetch_error_if_possible() const {
  int x = rpc_lookup_int();
  if (x == TL_RPC_REQ_ERROR && CurException.is_null()) {
    php_assert (tl_parse_int() == TL_RPC_REQ_ERROR);
    if (CurException.is_null()) {
      tl_parse_long();
      if (CurException.is_null()) {
        int error_code = tl_parse_int();
        if (CurException.is_null()) {
          string error = tl_parse_string();
          if (CurException.is_null()) {
            return make_error(error, error_code);
          }
        }
      }
    }
  }
  return make_error_from_exception_if_possible();
}
