// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <memory>

#include "runtime-light/allocator/allocator.h"
#include "runtime-light/core/kphp_core.h"
#include "runtime-light/tl/tl_func_base.h"
#include "runtime-light/tl/tl_rpc_function.h"

class RpcRequestResult : public dl::ManagedThroughDlAllocator {
public:
  const bool is_typed{};

  RpcRequestResult(bool is_typed, std::unique_ptr<tl_func_base> &&result_fetcher);

  bool empty() const;

  virtual class_instance<C$VK$TL$RpcResponse> fetch_typed_response() = 0;
  virtual std::unique_ptr<tl_func_base> extract_untyped_fetcher() = 0;
  virtual ~RpcRequestResult() = default;

protected:
  std::unique_ptr<tl_func_base> result_fetcher_; // the store() result
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
  class_instance<C$VK$TL$RpcFunction> storing_function_;
};

class RpcRequestResultUntyped final : public RpcRequestResult {
public:
  using RpcRequestResult::RpcRequestResult;

  explicit RpcRequestResultUntyped(std::unique_ptr<tl_func_base> &&result_fetcher);

  class_instance<C$VK$TL$RpcResponse> fetch_typed_response() final;

  std::unique_ptr<tl_func_base> extract_untyped_fetcher() final;
};
