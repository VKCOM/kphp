#pragma once
#include <utility>

#include "runtime/php_assert.h"
#include "runtime/unique_object.h"
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
    string tl_name {storing_function_.get()->get_class()};
    const string tl_class_prefix {"TL\\Functions\\"};
    const size_t pos = tl_name.find(tl_class_prefix);
    if (pos != string::npos) {
      tl_name = tl_name.substr(pos + tl_class_prefix.size(), tl_name.size() - (pos + tl_class_prefix.size()));
    }
    std::replace(tl_name.buffer(), tl_name.buffer() + tl_name.size(), '\\', '.');
    return tl_name;
  }

  bool empty() const { return storing_function_.is_null(); }

  virtual unique_object<RpcRequestResult> store_request() const = 0;
  virtual ~RpcRequest() = default;

protected:
  class_instance<C$VK$TL$RpcFunction> storing_function_;
};

class RpcRequestResult {
public:
  explicit RpcRequestResult(unique_object<tl_func_base> &&result_fetcher)
    : result_fetcher_(std::move(result_fetcher)) {}

  bool empty() const { return !result_fetcher_; }

  virtual class_instance<C$VK$TL$RpcResponse> fetch_typed_response() = 0;
  virtual unique_object<tl_func_base> extract_untyped_fetcher() = 0;
  virtual ~RpcRequestResult() = default;

protected:
  unique_object<tl_func_base> result_fetcher_; // то что возвращает store()
};

class RpcRequestResultUntyped final : public RpcRequestResult {
public:
  using RpcRequestResult::RpcRequestResult;

  class_instance<C$VK$TL$RpcResponse> fetch_typed_response() final {
    php_assert(!"Forbidden to call for non typed rpc requests");
    return {};
  }

  unique_object<tl_func_base> extract_untyped_fetcher() final {
    return std::move(result_fetcher_);
  }
};

namespace impl_ {
// use template, because t_ReqResult_ is unknown on runtime compilation
template<template<typename, unsigned int> class t_ReqResult_>
class KphpRpcRequestResult final : public RpcRequestResult {
public:
  using RpcRequestResult::RpcRequestResult;

  class_instance<C$VK$TL$RpcResponse> fetch_typed_response() final {
    class_instance<C$VK$TL$RpcResponse> $response;
    t_ReqResult_<tl_exclamation_fetch_wrapper, 0>(tl_exclamation_fetch_wrapper(std::move(result_fetcher_))).typed_fetch_to($response);
    return $response;
  }

  unique_object<tl_func_base> extract_untyped_fetcher() final {
    php_assert(!"Forbidden to call for typed rpc requests");
    return {};
  }
};

// use template, because t_ReqResult_ is unknown on runtime compilation
template<template<typename, unsigned int> class t_ReqResult_>
class KphpRpcRequest final : public RpcRequest {
public:
  using RpcRequest::RpcRequest;

  unique_object<RpcRequestResult> store_request() const final {
    php_assert(CurException.is_null());
    CurrentProcessingQuery::get().set_current_tl_function(tl_function_name());
    unique_object<tl_func_base> stored_fetcher = storing_function_.get()->store();
    CurrentProcessingQuery::get().reset();
    if (!CurException.is_null()) {
      CurException = false;
      return {};
    }
    return make_unique_object<KphpRpcRequestResult<t_ReqResult_>>(std::move(stored_fetcher));
  }
};
} // namespace impl_

template<class T0, unsigned int inner_magic0>
struct t_ReqResult;   // появляется после codegen из tl-схемы, при сборке сайта
using KphpRpcRequest = impl_::KphpRpcRequest<t_ReqResult>;
