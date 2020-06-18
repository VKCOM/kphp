#pragma once

#include "runtime/refcountable_php_classes.h"
#include "common/algorithms/hashes.h"
#include "common/wrappers/string_view.h"

struct tl_func_base;

class InstanceToArrayVisitor;
class InstanceMemoryEstimateVisitor;

// builtin-классы, которые описаны в functions.txt (связанные с типизированным TL)
// увы, здесь жёстко зашито, что они лежат именно в папке/namespace \VK\TL,
// т.к. после кодогенерации C$VK$TL$... должны соответствовать этой реализации

// этот интерфейс реализуют все tl-функции в php коде (см. tl-to-php)
struct C$VK$TL$RpcFunction : abstract_refcountable_php_interface {
  virtual const char *get_class() const { return "VK\\TL\\RpcFunction"; }
  virtual int get_hash() const { return static_cast<int>(vk::std_hash(vk::string_view(C$VK$TL$RpcFunction::get_class()))); }

  virtual void accept(InstanceToArrayVisitor &) {}
  virtual void accept(InstanceMemoryEstimateVisitor &) {}

  virtual ~C$VK$TL$RpcFunction() = default;
  virtual std::unique_ptr<tl_func_base> store() const = 0;
};

// у каждой tl-функции есть отдельный класс-результат implements RpcFunctionReturnResult,
// у которого есть ->value нужного типа 
struct C$VK$TL$RpcFunctionReturnResult : abstract_refcountable_php_interface {
  virtual const char *get_class() const { return "VK\\TL\\RpcFunctionReturnResult"; }
  virtual int get_hash() const { return static_cast<int>(vk::std_hash(vk::string_view(C$VK$TL$RpcFunctionReturnResult::get_class()))); }

  virtual void accept(InstanceToArrayVisitor &) {}
  virtual void accept(InstanceMemoryEstimateVisitor &) {}

  virtual ~C$VK$TL$RpcFunctionReturnResult() = default;
};

// ответ исполнения функции — ReqResult в tl-схеме — это rpcResponseOk|rpcResponseHeader|rpcResponseError
// (если это ok или header, то их body можно зафетчить тем фетчером, что вернул store)
struct C$VK$TL$RpcResponse : abstract_refcountable_php_interface {
  using X = class_instance<C$VK$TL$RpcFunctionReturnResult>;

  virtual void accept(InstanceToArrayVisitor &) {}
  virtual void accept(InstanceMemoryEstimateVisitor &) {}

  virtual const char *get_class() const { return "VK\\TL\\RpcResponse"; }
  virtual int get_hash() const { return static_cast<int>(vk::std_hash(vk::string_view(C$VK$TL$RpcResponse::get_class()))); }

  virtual ~C$VK$TL$RpcResponse() = default;
};
