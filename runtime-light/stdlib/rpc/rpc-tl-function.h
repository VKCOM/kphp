//  Compiler for PHP (aka KPHP)
//  Copyright (c) 2024 LLC «V Kontakte»
//  Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cstdint>

#include "common/algorithms/hashes.h"
#include "common/wrappers/string_view.h"
#include "runtime-common/core/class-instance/refcountable-php-classes.h"

struct tl_func_base;

class ToArrayVisitor;
class CommonMemoryEstimateVisitor;
class InstanceReferencesCountingVisitor;
class InstanceDeepCopyVisitor;
class InstanceDeepDestroyVisitor;

// The locations of the typed TL related builtin classes that are described in functions.txt
// are hardcoded to the folder/namespace \VK\TL because after the code generation
// C$VK$TL$... should match that layout

// this interface is implemented by all PHP classes that represent the TL functions (see tl-to-php)
struct C$VK$TL$RpcFunction : abstract_refcountable_php_interface {
  virtual const char *get_class() const {
    return "VK\\TL\\RpcFunction";
  }
  virtual int32_t get_hash() const {
    return static_cast<int32_t>(vk::std_hash(vk::string_view(C$VK$TL$RpcFunction::get_class())));
  }

  virtual void accept(ToArrayVisitor &) noexcept {}
  virtual void accept(CommonMemoryEstimateVisitor &) noexcept {}
  virtual void accept(InstanceReferencesCountingVisitor &) noexcept {}
  virtual void accept(InstanceDeepCopyVisitor &) noexcept {}
  virtual void accept(InstanceDeepDestroyVisitor &) noexcept {}

  virtual size_t virtual_builtin_sizeof() const noexcept {
    return 0;
  }
  virtual C$VK$TL$RpcFunction *virtual_builtin_clone() const noexcept {
    return nullptr;
  }

  ~C$VK$TL$RpcFunction() override = default;
  virtual std::unique_ptr<tl_func_base> store() const = 0;
};

// every TL function has a class for the result that implements RpcFunctionReturnResult;
// which has ->value of the required type
struct C$VK$TL$RpcFunctionReturnResult : abstract_refcountable_php_interface {
  virtual const char *get_class() const {
    return "VK\\TL\\RpcFunctionReturnResult";
  }
  virtual int32_t get_hash() const {
    return static_cast<int32_t>(vk::std_hash(vk::string_view(C$VK$TL$RpcFunctionReturnResult::get_class())));
  }

  virtual void accept(ToArrayVisitor &) noexcept {}
  virtual void accept(CommonMemoryEstimateVisitor &) noexcept {}
  virtual void accept(InstanceReferencesCountingVisitor &) noexcept {}
  virtual void accept(InstanceDeepCopyVisitor &) noexcept {}
  virtual void accept(InstanceDeepDestroyVisitor &) noexcept {}

  virtual size_t virtual_builtin_sizeof() const noexcept {
    return 0;
  }
  virtual C$VK$TL$RpcFunctionReturnResult *virtual_builtin_clone() const noexcept {
    return nullptr;
  }

  ~C$VK$TL$RpcFunctionReturnResult() override = default;
};

// function call response — ReqResult from the TL scheme — is a rpcResponseOk|rpcResponseHeader|rpcResponseError;
// if it's rpcResponseOk or rpcResponseHeader, then their bodies can be retrieved by a fetcher that was returned by a store
struct C$VK$TL$RpcResponse : abstract_refcountable_php_interface {
  using X = class_instance<C$VK$TL$RpcFunctionReturnResult>;

  virtual void accept(ToArrayVisitor &) noexcept {}
  virtual void accept(CommonMemoryEstimateVisitor &) noexcept {}
  virtual void accept(InstanceReferencesCountingVisitor &) noexcept {}
  virtual void accept(InstanceDeepCopyVisitor &) noexcept {}
  virtual void accept(InstanceDeepDestroyVisitor &) noexcept {}

  virtual const char *get_class() const {
    return "VK\\TL\\RpcResponse";
  }
  virtual int32_t get_hash() const {
    return static_cast<int32_t>(vk::std_hash(vk::string_view(C$VK$TL$RpcResponse::get_class())));
  }

  virtual size_t virtual_builtin_sizeof() const noexcept {
    return 0;
  }
  virtual C$VK$TL$RpcResponse *virtual_builtin_clone() const noexcept {
    return nullptr;
  }

  ~C$VK$TL$RpcResponse() override = default;
};
