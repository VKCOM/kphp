// Compiler for PHP (aka KPHP)
// Copyright (c) 2023 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime/tl/rpc_req_error.h"

#include "common/rpc-error-codes.h"
#include "common/tl/constants/common.h"
#include "runtime/rpc.h"

bool RpcError::try_fetch() noexcept {
  int op = rpc_lookup_int();
  if (op != TL_RPC_REQ_ERROR) {
    return false;
  }
  php_assert(tl_parse_int() == TL_RPC_REQ_ERROR);
  tl_parse_long();
  error_code = tl_parse_int();
  error_msg = tl_parse_string();

  if (!CurException.is_null()) {
    error_code = TL_ERROR_SYNTAX;
    error_msg = std::move(CurException->$message);
    CurException = Optional<bool>{};
  }
  return true;
}
