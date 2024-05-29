// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "kphp-core/class-instance/refcountable-php-classes.h"
#include "kphp-core/kphp_core.h"
#include "runtime/tl/rpc_function.h"

class RpcErrorFactory {
public:
  virtual class_instance<C$VK$TL$RpcResponse> make_error(const string &error, int error_code) const = 0;

  class_instance<C$VK$TL$RpcResponse> make_error(const char *error, int error_code) const;
  class_instance<C$VK$TL$RpcResponse> make_error_from_exception_if_possible() const;
  class_instance<C$VK$TL$RpcResponse> fetch_error_if_possible() const;

  virtual ~RpcErrorFactory() = default;
};

namespace impl_ {
// use template, because _common\Types\rpcResponseError is unknown on runtime compilation
template<class C$VK$TL$_common$Types$rpcResponseError_>
class rpcResponseErrorFactory : public RpcErrorFactory {
private:
  rpcResponseErrorFactory() = default;

  class_instance<C$VK$TL$RpcResponse> make_error(const string &error, int error_code) const final {
    class_instance<C$VK$TL$_common$Types$rpcResponseError_> err;
    err.alloc();
    err.get()->$error = error;
    err.get()->$error_code = error_code;
    return err;
  }

public:
  static const RpcErrorFactory &get() {
    const static rpcResponseErrorFactory self;
    return self;
  }
};
} // namespace impl_

// the definition appears after the TL scheme codegen, during the site build
struct C$VK$TL$_common$Types$rpcResponseError;
using rpcResponseErrorFactory = impl_::rpcResponseErrorFactory<C$VK$TL$_common$Types$rpcResponseError>;
