// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <memory>

#include "runtime-common/runtime-core/allocator/script-allocator-managed.h"
#include "runtime-common/runtime-core/runtime-core.h"
#include "runtime-light/stdlib/rpc/rpc-tl-func-base.h"
#include "runtime-light/stdlib/rpc/rpc-tl-function.h"

class RpcRequestResult : public ScriptAllocatorManaged {
public:
  const bool is_typed{};

  RpcRequestResult(bool is_typed, std::unique_ptr<tl_func_base> &&result_fetcher);

  bool empty() const;

  virtual class_instance<C$VK$TL$RpcResponse> fetch_typed_response() = 0;
  virtual std::unique_ptr<tl_func_base> extract_untyped_fetcher() = 0;
  virtual ~RpcRequestResult() = default;

protected:
  std::unique_ptr<tl_func_base> result_fetcher; // the store() result
};

class RpcRequest {
public:
  explicit RpcRequest(class_instance<C$VK$TL$RpcFunction> function);

  string tl_function_name() const;

  bool empty() const;

  const class_instance<C$VK$TL$RpcFunction> &get_tl_function() const;

  virtual std::unique_ptr<RpcRequestResult> store_request() const = 0;
  virtual ~RpcRequest() = default;

protected:
  class_instance<C$VK$TL$RpcFunction> storing_function;
};

class RpcRequestResultUntyped final : public RpcRequestResult {
public:
  using RpcRequestResult::RpcRequestResult;

  explicit RpcRequestResultUntyped(std::unique_ptr<tl_func_base> &&result_fetcher);

  class_instance<C$VK$TL$RpcResponse> fetch_typed_response() final;

  std::unique_ptr<tl_func_base> extract_untyped_fetcher() final;
};
