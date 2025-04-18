//  Compiler for PHP (aka KPHP)
//  Copyright (c) 2024 LLC «V Kontakte»
//  Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cstdint>

#include "common/rpc-error-codes.h"
#include "runtime-light/stdlib/diagnostics/exception-types.h"
#include "runtime-light/stdlib/fork/fork-state.h"

#include "runtime-common/core/runtime-core.h"
#include "runtime-light/stdlib/rpc/rpc-tl-function.h"

struct TlRpcError {
  int32_t error_code{0};
  string error_msg;

  array<mixed> make_error() noexcept {
    array<mixed> res;
    res.set_value(string{"__error", 7}, error_msg);
    res.set_value(string{"__error_code", 12}, error_code);
    return res;
  }

  static array<mixed> make_error(int32_t error_code, string error_msg) noexcept {
    return TlRpcError{.error_code = error_code, .error_msg = std::move(error_msg)}.make_error();
  }

  static array<mixed> transform_exception_into_error_if_possible() noexcept {
    if (auto& cur_fork_info{ForkInstanceState::get().current_info().get()}; !cur_fork_info.thrown_exception.is_null()) {
      TlRpcError err{.error_code = TL_ERROR_SYNTAX, .error_msg = std::move(cur_fork_info.thrown_exception.get()->$message)};
      cur_fork_info.thrown_exception = Throwable{};
      return err.make_error();
    }
    return {};
  }

  bool try_fetch() noexcept;

private:
  void fetch_and_skip_header() const noexcept;
};

class RpcErrorFactory {
public:
  virtual class_instance<C$VK$TL$RpcResponse> make_error(TlRpcError error) const noexcept = 0;

  class_instance<C$VK$TL$RpcResponse> make_error(int32_t error_code, string error_msg) const noexcept {
    return make_error(TlRpcError{.error_code = error_code, .error_msg = std::move(error_msg)});
  }

  class_instance<C$VK$TL$RpcResponse> transform_exception_into_error_if_possible() const noexcept {
    if (auto& cur_fork_info{ForkInstanceState::get().current_info().get()}; !cur_fork_info.thrown_exception.is_null()) {
      TlRpcError err{.error_code = TL_ERROR_SYNTAX, .error_msg = std::move(cur_fork_info.thrown_exception.get()->$message)};
      cur_fork_info.thrown_exception = Throwable{};
      return make_error(std::move(err));
    }
    return {};
  }

  virtual ~RpcErrorFactory() = default;
};

namespace kphp::rpc::rpc_impl {

// use template, because _common\Types\rpcResponseError is unknown on runtime compilation
template<typename C$VK$TL$_common$Types$rpcResponseError_>
struct RpcResponseErrorFactory : public RpcErrorFactory {
  RpcResponseErrorFactory() = default;

private:
  class_instance<C$VK$TL$RpcResponse> make_error(TlRpcError error) const noexcept final {
    auto typed_err{make_instance<C$VK$TL$_common$Types$rpcResponseError_>()};
    typed_err.get()->$error = std::move(error.error_msg);
    typed_err.get()->$error_code = error.error_code;
    return typed_err;
  }
};

} // namespace kphp::rpc::rpc_impl

// the definition appears after the TL scheme codegen, during the site build
struct C$VK$TL$_common$Types$rpcResponseError;
using RpcResponseErrorFactory = kphp::rpc::rpc_impl::RpcResponseErrorFactory<C$VK$TL$_common$Types$rpcResponseError>;
