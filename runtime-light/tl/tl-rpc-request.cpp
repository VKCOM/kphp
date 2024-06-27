//  Compiler for PHP (aka KPHP)
//  Copyright (c) 2024 LLC «V Kontakte»
//  Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime-light/tl/tl-rpc-request.h"

#include "runtime-light/utils/php_assert.h"

RpcRequestResult::RpcRequestResult(bool is_typed, std::unique_ptr<tl_func_base> &&result_fetcher)
  : is_typed(is_typed)
  , result_fetcher(std::move(result_fetcher)) {}

bool RpcRequestResult::empty() const {
  return !result_fetcher;
}

RpcRequest::RpcRequest(class_instance<C$VK$TL$RpcFunction> function)
  : storing_function(std::move(function)) {}

string RpcRequest::tl_function_name() const {
  string class_name{storing_function.get()->get_class()};
  const string tl_class_prefix{"\\Functions\\"};
  const auto pos = class_name.find(tl_class_prefix);
  if (pos != string::npos) {
    class_name = class_name.substr(pos + tl_class_prefix.size(), class_name.size() - (pos + tl_class_prefix.size()));
  }
  return class_name;
}

bool RpcRequest::empty() const {
  return storing_function.is_null();
}

const class_instance<C$VK$TL$RpcFunction> &RpcRequest::get_tl_function() const {
  return storing_function;
}

RpcRequestResultUntyped::RpcRequestResultUntyped(std::unique_ptr<tl_func_base> &&result_fetcher)
  : RpcRequestResult(false, std::move(result_fetcher)) {}

class_instance<C$VK$TL$RpcResponse> RpcRequestResultUntyped::fetch_typed_response() {
  php_assert(!"Forbidden to call for non typed rpc requests");
}

std::unique_ptr<tl_func_base> RpcRequestResultUntyped::extract_untyped_fetcher() {
  return std::move(result_fetcher);
}
