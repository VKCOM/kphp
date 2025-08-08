//  Compiler for PHP (aka KPHP)
//  Copyright (c) 2024 LLC «V Kontakte»
//  Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "runtime-common/core/allocator/script-allocator-managed.h"
#include "runtime-common/core/runtime-core.h"
#include "runtime-light/stdlib/diagnostics/diagnostics.h"
#include "runtime-light/stdlib/rpc/rpc-tl-function.h"

struct tl_func_base : ScriptAllocatorManaged {
  virtual mixed fetch() = 0;

  virtual class_instance<C$VK$TL$RpcFunctionReturnResult> typed_fetch() {
    // all functions that are called in a typed way override this method with the generated code;
    // functions that are not called in a typed way will never call this method
    // (it's not a pure virtual method so it's not necessary to generate "return {};" for the untyped functions)
    kphp::log::error("This function should never be called. Should be overridden in every TL function used in typed mode");
    return {};
  }

  virtual void rpc_server_typed_store([[maybe_unused]] const class_instance<C$VK$TL$RpcFunctionReturnResult>& res) {
    // all functions annotated with @kphp will override this method with the generated code
    kphp::log::error("This function should never be called. Should be overridden in every @kphp TL function");
  }

  // every TL function in C++ also has:
  // static std::unique_ptr<tl_func_base> store(const mixed &tl_object);
  // static std::unique_ptr<tl_func_base> typed_store(const C$VK$TL$Functions$thisfunction *tl_object);
  // they are not virtual (as they're static), but the implementation is generated for every class
  // every one of them creates an instance of itself (fetcher) which is used to do a fetch()/typed_fetch() when the response is received

  virtual ~tl_func_base() = default;
};
