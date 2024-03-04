// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once
#include <utility>

#include "runtime/php_assert.h"
#include "runtime/tl/rpc_function.h"
#include "runtime/tl/rpc_response.h"
#include "runtime/tl/tl_builtins.h"

class RpcRequestResult;

class RpcRequest {
public:
  explicit RpcRequest(class_instance<C$VK$TL$RpcFunction> function) :
    storing_function_(std::move(function)) {
  }

  string tl_function_name() const {
    string class_name{storing_function_.get()->get_class()};
    const string tl_class_prefix{"\\Functions\\"};
    const auto pos = class_name.find(tl_class_prefix);
    if (pos != string::npos) {
      class_name = class_name.substr(pos + tl_class_prefix.size(), class_name.size() - (pos + tl_class_prefix.size()));
    }
    // "messages_getChatInfo", "smart_alerts_sendMessage" — without a dot (unlike in TL), but it's still valid for logs/1kw
    return class_name;
  }

  bool empty() const { return storing_function_.is_null(); }
  const class_instance<C$VK$TL$RpcFunction>& get_tl_function() const { return storing_function_; }

  virtual std::unique_ptr<RpcRequestResult> store_request() const = 0;
  virtual ~RpcRequest() = default;

protected:
  class_instance<C$VK$TL$RpcFunction> storing_function_;
};

class RpcRequestResult : public ManagedThroughDlAllocator {
public:
  const bool is_typed{};

  RpcRequestResult(bool is_typed, std::unique_ptr<tl_func_base> &&result_fetcher)
    : is_typed(is_typed)
    , result_fetcher_(std::move(result_fetcher)) {}

  bool empty() const {
    return !result_fetcher_;
  }

  virtual class_instance<C$VK$TL$RpcResponse> fetch_typed_response() = 0;
  virtual std::unique_ptr<tl_func_base> extract_untyped_fetcher() = 0;
  virtual ~RpcRequestResult() = default;

protected:
  std::unique_ptr<tl_func_base> result_fetcher_; // the store() result
};

class RpcRequestResultUntyped final : public RpcRequestResult {
public:
  using RpcRequestResult::RpcRequestResult;

  explicit RpcRequestResultUntyped(std::unique_ptr<tl_func_base> &&result_fetcher)
    : RpcRequestResult(false, std::move(result_fetcher)) {}

  class_instance<C$VK$TL$RpcResponse> fetch_typed_response() final {
    php_assert(!"Forbidden to call for non typed rpc requests");
    return {};
  }

  std::unique_ptr<tl_func_base> extract_untyped_fetcher() final {
    return std::move(result_fetcher_);
  }
};

namespace impl_ {
// use template, because t_ReqResult_ is unknown on runtime compilation
template<template<typename, unsigned int> class t_ReqResult_>
class KphpRpcRequestResult final : public RpcRequestResult {
public:
  using RpcRequestResult::RpcRequestResult;

  explicit KphpRpcRequestResult(std::unique_ptr<tl_func_base> &&result_fetcher)
    : RpcRequestResult(true, std::move(result_fetcher)) {}

  class_instance<C$VK$TL$RpcResponse> fetch_typed_response() final {
    class_instance<C$VK$TL$RpcResponse> $response;
    t_ReqResult_<tl_exclamation_fetch_wrapper, 0>(tl_exclamation_fetch_wrapper(std::move(result_fetcher_))).typed_fetch_to($response);
    return $response;
  }

  std::unique_ptr<tl_func_base> extract_untyped_fetcher() final {
    php_assert(!"Forbidden to call for typed rpc requests");
    return {};
  }
};

// use template, because t_ReqResult_ is unknown on runtime compilation
template<template<typename, unsigned int> class t_ReqResult_>
class KphpRpcRequest final : public RpcRequest {
public:
  using RpcRequest::RpcRequest;

  std::unique_ptr<RpcRequestResult> store_request() const final {
    php_assert(CurException.is_null());
    CurrentProcessingQuery::get().set_current_tl_function(tl_function_name());
    std::unique_ptr<tl_func_base> stored_fetcher = storing_function_.get()->store();
    CurrentProcessingQuery::get().reset();
    if (!CurException.is_null()) {
      CurException = Optional<bool>{};
      return {};
    }
    return make_unique_on_script_memory<KphpRpcRequestResult<t_ReqResult_>>(std::move(stored_fetcher));
  }
};
} // namespace impl_

template<class T0, unsigned int inner_magic0>
struct t_ReqResult;   // the definition appears after the TL scheme codegen, during the site build
//
using KphpRpcRequest = impl_::KphpRpcRequest<t_ReqResult>;
