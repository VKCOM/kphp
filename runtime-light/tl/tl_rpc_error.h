//  Compiler for PHP (aka KPHP)
//  Copyright (c) 2024 LLC «V Kontakte»
//  Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cstdint>

#include "runtime-light/core/kphp_core.h"
#include "runtime-light/tl/tl_rpc_function.h"

struct TlRpcError {
  int32_t error_code{0};
  string error_msg;

  bool try_fetch() noexcept;

private:
  void fetch_and_skip_header() const noexcept;
};

class RpcErrorFactory {
public:
  virtual class_instance<C$VK$TL$RpcResponse> make_error(const string &error, int32_t error_code) const noexcept = 0;

  class_instance<C$VK$TL$RpcResponse> make_error(const char *error, int32_t error_code) const noexcept;
  class_instance<C$VK$TL$RpcResponse> make_error_from_exception_if_possible() const noexcept;
  class_instance<C$VK$TL$RpcResponse> fetch_error_if_possible() const noexcept;

  virtual ~RpcErrorFactory() = default;
};

namespace tl_rpc_error_impl_ {

// use template, because _common\Types\rpcResponseError is unknown on runtime compilation
template<typename C$VK$TL$_common$Types$rpcResponseError_>
class RpcResponseErrorFactory : public RpcErrorFactory {
private:
  RpcResponseErrorFactory() = default;

  class_instance<C$VK$TL$RpcResponse> make_error(const string &error, int32_t error_code) const noexcept final {
    auto err{make_instance<C$VK$TL$_common$Types$rpcResponseError_>()};
    err.get()->$error = error;
    err.get()->$error_code = error_code;
    return err;
  }
};

} // namespace tl_rpc_error_impl_

// the definition appears after the TL scheme codegen, during the site build
struct C$VK$TL$_common$Types$rpcResponseError;
using RpcResponseErrorFactory = tl_rpc_error_impl_::RpcResponseErrorFactory<C$VK$TL$_common$Types$rpcResponseError>;
