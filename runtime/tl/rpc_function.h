// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "runtime/refcountable_php_classes.h"
#include "common/algorithms/hashes.h"
#include "common/wrappers/string_view.h"

struct tl_func_base;

class InstanceToArrayVisitor;
class InstanceMemoryEstimateVisitor;

// The locations of the typed TL related builtin classes that are described in functions.txt
// are hardcoded to the folder/namespace \VK\TL because after the code generation
// C$VK$TL$... should match that layout

// this interface is implemented by all PHP classes that represent the TL functions (see tl-to-php)
struct C$VK$TL$RpcFunction : abstract_refcountable_php_interface {
  virtual const char *get_class() const { return "VK\\TL\\RpcFunction"; }
  virtual int32_t get_hash() const { return static_cast<int32_t>(vk::std_hash(vk::string_view(C$VK$TL$RpcFunction::get_class()))); }

  virtual void accept(InstanceToArrayVisitor &) {}
  virtual void accept(InstanceMemoryEstimateVisitor &) {}

  virtual ~C$VK$TL$RpcFunction() = default;
  virtual std::unique_ptr<tl_func_base> store() const = 0;
};

// every TL function has a class for the result that implements RpcFunctionReturnResult;
// which has ->value of the required type
struct C$VK$TL$RpcFunctionReturnResult : abstract_refcountable_php_interface {
  virtual const char *get_class() const { return "VK\\TL\\RpcFunctionReturnResult"; }
  virtual int32_t get_hash() const { return static_cast<int32_t>(vk::std_hash(vk::string_view(C$VK$TL$RpcFunctionReturnResult::get_class()))); }

  virtual void accept(InstanceToArrayVisitor &) {}
  virtual void accept(InstanceMemoryEstimateVisitor &) {}

  virtual ~C$VK$TL$RpcFunctionReturnResult() = default;
};

// function call response — ReqResult from the TL scheme — is a rpcResponseOk|rpcResponseHeader|rpcResponseError;
// if it's rpcResponseOk or rpcResponseHeader, then their bodies can be retrieved by a fetcher that was returned by a store
struct C$VK$TL$RpcResponse : abstract_refcountable_php_interface {
  using X = class_instance<C$VK$TL$RpcFunctionReturnResult>;

  virtual void accept(InstanceToArrayVisitor &) {}
  virtual void accept(InstanceMemoryEstimateVisitor &) {}

  virtual const char *get_class() const { return "VK\\TL\\RpcResponse"; }
  virtual int32_t get_hash() const { return static_cast<int32_t>(vk::std_hash(vk::string_view(C$VK$TL$RpcResponse::get_class()))); }

  virtual ~C$VK$TL$RpcResponse() = default;
};
