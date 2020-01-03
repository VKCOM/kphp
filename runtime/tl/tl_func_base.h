#pragma once
#include "runtime/kphp_core.h"
#include "runtime/tl/rpc_function.h"

struct tl_func_base : ManagedThroughDlAllocator {
  virtual var fetch() = 0;

  virtual class_instance<C$VK$TL$RpcFunctionReturnResult> typed_fetch() {
    // все функции, вызывающиеся типизированно, кодогенерированно переопределяют этот метод
    // а функции, типизированно не вызывающиеся, никогда не будут вызваны
    // (не стали делать её чистой виртуальной, чтобы для не типизированных не переопределять на "return {};")
    php_critical_error("This function should never be called. Should be overridden in every TL function used in typed mode");
    return {};
  }

  virtual void rpc_server_typed_store(const class_instance<C$VK$TL$RpcFunctionReturnResult> &) {
    // все функции, помеченные аннотацией @kphp, кодогенерированно переопределяют этот метод
    php_critical_error("This function should never be called. Should be overridden in every @kphp TL function");
  }

  // каждая плюсовая tl-функция ещё обладает
  // static std::unique_ptr<tl_func_base> store(const var &tl_object);
  // static std::unique_ptr<tl_func_base> typed_store(const C$VK$TL$Functions$thisfunction *tl_object);
  // они не виртуальные, т.к. static, но кодогенерятся в каждой
  // каждая из них создаёт инстанс себя (fetcher), на котором вызываются fetch()/typed_fetch(), когда ответ получен

  virtual ~tl_func_base() = default;
};