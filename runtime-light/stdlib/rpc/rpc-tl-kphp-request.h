//  Compiler for PHP (aka KPHP)
//  Copyright (c) 2024 LLC «V Kontakte»
//  Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cstdint>
#include <memory>

#include "common/containers/final_action.h"
#include "runtime-light/allocator/allocator.h"
#include "runtime-light/stdlib/diagnostics/exception-functions.h"
#include "runtime-light/stdlib/diagnostics/logs.h"
#include "runtime-light/stdlib/rpc/rpc-tl-defs.h"
#include "runtime-light/stdlib/rpc/rpc-tl-func-base.h"
#include "runtime-light/stdlib/rpc/rpc-tl-query.h"
#include "runtime-light/stdlib/rpc/rpc-tl-request.h"

class_instance<C$VK$TL$RpcFunctionFetcher> f$VK$TL$RpcFunction$$customStore(class_instance<C$VK$TL$RpcFunction> const& arg) noexcept;
class_instance<C$VK$TL$RpcFunctionFetcher> f$VK$TL$RpcFunction$$customFetch(class_instance<C$VK$TL$RpcFunction> const& arg) noexcept;
class_instance<C$VK$TL$RpcFunctionReturnResult> f$RpcFunctionFetcher$$typedFetch(class_instance<C$VK$TL$RpcFunctionFetcher> const& fetcher) noexcept;
void f$RpcFunctionFetcher$$typedStore(class_instance<C$VK$TL$RpcFunctionFetcher> const& fetcher,
                                      class_instance<C$VK$TL$RpcFunctionReturnResult> const& result) noexcept;

// should be in header, because C$VK$TL$* classes are unknown on runtime compilation
struct tl_func_base_simple_wrapper : public tl_func_base {
  explicit tl_func_base_simple_wrapper(class_instance<C$VK$TL$RpcFunctionFetcher>&& wrapped)
      : wrapped_(std::move(wrapped)) {}

  virtual mixed fetch() {
    php_critical_error("this function should never be called for typed RPC function.");
    return mixed{};
  }

  virtual class_instance<C$VK$TL$RpcFunctionReturnResult> typed_fetch() {
    return f$RpcFunctionFetcher$$typedFetch(wrapped_);
  }

  virtual void rpc_server_typed_store(const class_instance<C$VK$TL$RpcFunctionReturnResult>& result) {
    return f$RpcFunctionFetcher$$typedStore(wrapped_, result);
  }

private:
  class_instance<C$VK$TL$RpcFunctionFetcher> wrapped_;
};

inline std::unique_ptr<tl_func_base> make_tl_func_base_simple_wrapper(class_instance<C$VK$TL$RpcFunctionFetcher>&& wrapped) {
  return std::make_unique<tl_func_base_simple_wrapper>(std::move(wrapped));
}

namespace kphp::rpc::rpc_impl {
// use template, because t_ReqResult_ is unknown on runtime compilation
template<template<typename, uint32_t> class t_ReqResult_>
class KphpRpcRequestResult final : public RpcRequestResult {
public:
  using RpcRequestResult::RpcRequestResult;

  explicit KphpRpcRequestResult(std::unique_ptr<tl_func_base>&& result_fetcher) noexcept
      : RpcRequestResult(true, std::move(result_fetcher)) {}

  class_instance<C$VK$TL$RpcResponse> fetch_typed_response() noexcept final {
    class_instance<C$VK$TL$RpcResponse> $response;
    t_ReqResult_<tl_exclamation_fetch_wrapper, 0>(tl_exclamation_fetch_wrapper(std::move(result_fetcher))).typed_fetch_to($response);
    return $response;
  }

  std::unique_ptr<tl_func_base> extract_untyped_fetcher() noexcept final {
    kphp::log::error("Forbidden to call for typed rpc requests");
  }
};

// use template, because t_ReqResult_ is unknown on runtime compilation
template<template<typename, uint32_t> class t_ReqResult_>
class KphpRpcRequest final : public RpcRequest {
public:
  using RpcRequest::RpcRequest;

  std::unique_ptr<RpcRequestResult> store_request() const noexcept final {
    CHECK_EXCEPTION(kphp::log::assertion(false));
    auto& cur_query{CurrentTlQuery::get()};
    cur_query.set_current_tl_function(tl_function_name());
    const vk::final_action finalizer{[&cur_query] noexcept { cur_query.reset(); }};
    std::unique_ptr<tl_func_base> stored_fetcher;
    auto custom_fetcher = f$VK$TL$RpcFunction$$customStore(storing_function);
    if (custom_fetcher.is_null()) {
      stored_fetcher = storing_function.get()->store();
    } else {
      stored_fetcher = make_tl_func_base_simple_wrapper(std::move(custom_fetcher));
    }
    CHECK_EXCEPTION(return {});
    return make_unique_on_script_memory<KphpRpcRequestResult<t_ReqResult_>>(std::move(stored_fetcher));
  }
};
} // namespace kphp::rpc::rpc_impl

template<class T0, uint32_t inner_magic0>
struct t_ReqResult; // the definition appears after the TL scheme codegen, during the site build

using KphpRpcRequest = kphp::rpc::rpc_impl::KphpRpcRequest<t_ReqResult>;
