#pragma once

#include "runtime/kphp_core.h"
#include "runtime/refcountable_php_classes.h"
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
// use template, because C$rpcResponseError is unknown on runtime compilation
template<class C$rpcResponseError_>
class rpcResponseErrorFactory : public RpcErrorFactory {
  class_instance<C$VK$TL$RpcResponse> make_error(const string &error, int error_code) const final {
    class_instance<C$rpcResponseError_> err;
    err.alloc();
    err.get()->$error = error;
    err.get()->$error_code = error_code;
    return err;
  }
};
} // namespace impl_

struct C$VK$TL$Types$rpcResponseError;   // появляется после codegen из tl-схемы, при сборке сайта
using rpcResponseErrorFactory = impl_::rpcResponseErrorFactory<C$VK$TL$Types$rpcResponseError>;
