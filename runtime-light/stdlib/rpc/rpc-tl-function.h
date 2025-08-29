//  Compiler for PHP (aka KPHP)
//  Copyright (c) 2024 LLC «V Kontakte»
//  Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cstdint>
#include <string_view>

#include "common/algorithms/hashes.h"
#include "runtime-common/core/class-instance/refcountable-php-classes.h"
#include "runtime-common/core/runtime-core.h"

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
  virtual const char* get_class() const noexcept {
    return "VK\\TL\\RpcFunction";
  }

  virtual int32_t get_hash() const noexcept {
    std::string_view name_view{C$VK$TL$RpcFunction::get_class()};
    return static_cast<int32_t>(vk::murmur_hash<uint32_t>(name_view.data(), name_view.size()));
  }

  virtual void accept(ToArrayVisitor& /*unused*/) noexcept {}
  virtual void accept(CommonMemoryEstimateVisitor& /*unused*/) noexcept {}
  virtual void accept(InstanceReferencesCountingVisitor& /*unused*/) noexcept {}
  virtual void accept(InstanceDeepCopyVisitor& /*unused*/) noexcept {}
  virtual void accept(InstanceDeepDestroyVisitor& /*unused*/) noexcept {}

  virtual size_t virtual_builtin_sizeof() const noexcept {
    return 0;
  }
  virtual C$VK$TL$RpcFunction* virtual_builtin_clone() const noexcept {
    return nullptr;
  }

  ~C$VK$TL$RpcFunction() override = default;
  virtual std::unique_ptr<tl_func_base> store() const = 0;
};

// every TL function has a class for the result that implements RpcFunctionReturnResult;
// which has ->value of the required type
struct C$VK$TL$RpcFunctionReturnResult : abstract_refcountable_php_interface {
  virtual const char* get_class() const noexcept {
    return "VK\\TL\\RpcFunctionReturnResult";
  }
  virtual int32_t get_hash() const noexcept {
    std::string_view name_view{C$VK$TL$RpcFunctionReturnResult::get_class()};
    return static_cast<int32_t>(vk::murmur_hash<uint32_t>(name_view.data(), name_view.size()));
  }

  virtual void accept(ToArrayVisitor& /*unused*/) noexcept {}
  virtual void accept(CommonMemoryEstimateVisitor& /*unused*/) noexcept {}
  virtual void accept(InstanceReferencesCountingVisitor& /*unused*/) noexcept {}
  virtual void accept(InstanceDeepCopyVisitor& /*unused*/) noexcept {}
  virtual void accept(InstanceDeepDestroyVisitor& /*unused*/) noexcept {}

  virtual size_t virtual_builtin_sizeof() const noexcept {
    return 0;
  }
  virtual C$VK$TL$RpcFunctionReturnResult* virtual_builtin_clone() const noexcept {
    return nullptr;
  }

  ~C$VK$TL$RpcFunctionReturnResult() override = default;
};

struct C$RpcFunctionFetcher : abstract_refcountable_php_interface {
  virtual const char* get_class() const {
    return "RpcFunctionFetcher";
  }
  virtual int32_t get_hash() const {
    std::string_view name_view{C$RpcFunctionFetcher::get_class()};
    return static_cast<int32_t>(vk::murmur_hash<uint32_t>(name_view.data(), name_view.size()));
  }
  virtual C$RpcFunctionFetcher* virtual_builtin_clone() const noexcept {
    return nullptr;
  }

  virtual class_instance<C$VK$TL$RpcFunctionReturnResult> typed_fetch() {
    php_warning("C$RpcFunctionFetcher::typed_fetch should never be called. Should be overridden in every @kphp TL function fetcher");
    return class_instance<C$VK$TL$RpcFunctionReturnResult>{};
  }
  virtual ~C$RpcFunctionFetcher() = default;
};

// function call response — ReqResult from the TL scheme — is a rpcResponseOk|rpcResponseHeader|rpcResponseError;
// if it's rpcResponseOk or rpcResponseHeader, then their bodies can be retrieved by a fetcher that was returned by a store
struct C$VK$TL$RpcResponse : abstract_refcountable_php_interface {
  using X = class_instance<C$VK$TL$RpcFunctionReturnResult>;

  virtual void accept(ToArrayVisitor& /*unused*/) noexcept {}
  virtual void accept(CommonMemoryEstimateVisitor& /*unused*/) noexcept {}
  virtual void accept(InstanceReferencesCountingVisitor& /*unused*/) noexcept {}
  virtual void accept(InstanceDeepCopyVisitor& /*unused*/) noexcept {}
  virtual void accept(InstanceDeepDestroyVisitor& /*unused*/) noexcept {}

  virtual const char* get_class() const noexcept {
    return "VK\\TL\\RpcResponse";
  }
  virtual int32_t get_hash() const noexcept {
    std::string_view name_view{C$VK$TL$RpcResponse::get_class()};
    return static_cast<int32_t>(vk::murmur_hash<uint32_t>(name_view.data(), name_view.size()));
  }

  virtual size_t virtual_builtin_sizeof() const noexcept {
    return 0;
  }
  virtual C$VK$TL$RpcResponse* virtual_builtin_clone() const noexcept {
    return nullptr;
  }

  ~C$VK$TL$RpcResponse() override = default;
};
