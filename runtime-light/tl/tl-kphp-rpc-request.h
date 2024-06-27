//  Compiler for PHP (aka KPHP)
//  Copyright (c) 2024 LLC «V Kontakte»
//  Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cstdint>
#include <memory>

#include "runtime-core/allocator/script-allocator-managed.h"
#include "runtime-light/stdlib/rpc/rpc-context.h"
#include "runtime-light/tl/tl-defs.h"
#include "runtime-light/tl/tl-rpc-request.h"

namespace tl_rpc_request_impl_ {
// use template, because t_ReqResult_ is unknown on runtime compilation
template<template<typename, uint32_t> class t_ReqResult_>
class KphpRpcRequestResult final : public RpcRequestResult {
public:
  using RpcRequestResult::RpcRequestResult;

  explicit KphpRpcRequestResult(std::unique_ptr<tl_func_base> &&result_fetcher)
    : RpcRequestResult(true, std::move(result_fetcher)) {}

  class_instance<C$VK$TL$RpcResponse> fetch_typed_response() final {
    class_instance<C$VK$TL$RpcResponse> $response;
    t_ReqResult_<tl_exclamation_fetch_wrapper, 0>(tl_exclamation_fetch_wrapper(std::move(result_fetcher))).typed_fetch_to($response);
    return $response;
  }

  std::unique_ptr<tl_func_base> extract_untyped_fetcher() final {
    php_assert(!"Forbidden to call for typed rpc requests");
  }
};

// use template, because t_ReqResult_ is unknown on runtime compilation
template<template<typename, uint32_t> class t_ReqResult_>
class KphpRpcRequest final : public RpcRequest {
public:
  using RpcRequest::RpcRequest;

  std::unique_ptr<RpcRequestResult> store_request() const final {
    //    php_assert(CurException.is_null());
    auto &rpc_ctx{RpcComponentContext::current()};
    rpc_ctx.current_query.set_current_tl_function(tl_function_name());
    std::unique_ptr<tl_func_base> stored_fetcher = storing_function.get()->store();
    rpc_ctx.current_query.reset();
    //    if (!CurException.is_null()) {
    //      CurException = Optional<bool>{};
    //      return {};
    //    }

    using kphp_rpc_request_result_t = KphpRpcRequestResult<t_ReqResult_>;
    static_assert(std::is_base_of_v<ScriptAllocatorManaged, kphp_rpc_request_result_t>);
    return std::make_unique<kphp_rpc_request_result_t>(std::move(stored_fetcher));
  }
};
} // namespace tl_rpc_request_impl_

template<class T0, uint32_t inner_magic0>
struct t_ReqResult; // the definition appears after the TL scheme codegen, during the site build

using KphpRpcRequest = tl_rpc_request_impl_::KphpRpcRequest<t_ReqResult>;
