//  Compiler for PHP (aka KPHP)
//  Copyright (c) 2024 LLC «V Kontakte»
//  Distributed under the GPL v3 License, see LICENSE.notice.txt


#pragma once

#include "runtime-core/runtime-core.h"

#define THROW_EXCEPTION(e) {php_critical_error("Exceptions unsupported");}

#define CHECK_EXCEPTION(action)

#ifdef __clang__
  #define TRY_CALL_RET_(x) x
#else
  #define TRY_CALL_RET_(x) std::move(x)
#endif

#define TRY_CALL_(CallT, call, action) ({ \
CallT x_tmp___ = (call);                \
CHECK_EXCEPTION(action);                \
TRY_CALL_RET_(x_tmp___);                \
})

#define TRY_CALL_VOID_(call, action) ({(call); CHECK_EXCEPTION(action); void();})

#define TRY_CALL(CallT, ResT, call) TRY_CALL_(CallT, call, return (ResT()))
#define TRY_CALL_VOID(ResT, call) TRY_CALL_VOID_(call, return (ResT()))

#define TRY_CALL_EXIT(CallT, message, call) TRY_CALL_(CallT, call, php_critical_error (message))
#define TRY_CALL_VOID_EXIT(message, call) TRY_CALL_VOID_(call, php_critical_error (message))


template<typename T>
T f$_exception_set_location(const T &e, const string &file, int64_t line) {
  php_critical_error("call to unsupported function");
}
