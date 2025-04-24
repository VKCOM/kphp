//  Compiler for PHP (aka KPHP)
//  Copyright (c) 2024 LLC «V Kontakte»
//  Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cstdint>
#include <memory>

#include "common/containers/final_action.h"
#include "runtime-common/core/utils/kphp-assert-core.h"
#include "runtime-light/allocator/allocator.h"
#include "runtime-light/stdlib/diagnostics/exception-functions.h"
#include "runtime-light/stdlib/rpc/rpc-tl-defs.h"
#include "runtime-light/stdlib/rpc/rpc-tl-query.h"
#include "runtime-light/stdlib/rpc/rpc-tl-request.h"

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
    php_assert(!"Forbidden to call for typed rpc requests");
  }
};

// use template, because t_ReqResult_ is unknown on runtime compilation
template<template<typename, uint32_t> class t_ReqResult_>
class KphpRpcRequest final : public RpcRequest {
public:
  using RpcRequest::RpcRequest;

  std::unique_ptr<RpcRequestResult> store_request() const noexcept final {
    CHECK_EXCEPTION(php_assert(false));
    auto& cur_query{CurrentTlQuery::get()};
    cur_query.set_current_tl_function(tl_function_name());
    const vk::final_action finalizer{[&cur_query] noexcept { cur_query.reset(); }};

    std::unique_ptr<tl_func_base> stored_fetcher{storing_function.get()->store()};
    CHECK_EXCEPTION(return {});
    return make_unique_on_script_memory<KphpRpcRequestResult<t_ReqResult_>>(std::move(stored_fetcher));
  }
};
} // namespace kphp::rpc::rpc_impl

template<class T0, uint32_t inner_magic0>
struct t_ReqResult; // the definition appears after the TL scheme codegen, during the site build

using KphpRpcRequest = kphp::rpc::rpc_impl::KphpRpcRequest<t_ReqResult>;
