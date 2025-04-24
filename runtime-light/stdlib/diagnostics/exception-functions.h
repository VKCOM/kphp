//  Compiler for PHP (aka KPHP)
//  Copyright (c) 2024 LLC «V Kontakte»
//  Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <concepts>
#include <cstdint>

#include "runtime-common/core/runtime-core.h"
#include "runtime-light/stdlib/diagnostics/exception-types.h"
#include "runtime-light/stdlib/fork/fork-state.h" // it's actually used by exception handling stuff

#define THROW_EXCEPTION(e)                                                                                                                                     \
  {                                                                                                                                                            \
    Throwable x_tmp___ = e;                                                                                                                                    \
    php_assert(std::exchange(ForkInstanceState::get().current_info().get().thrown_exception, std::move(x_tmp___)).is_null());                                  \
  }

#define CHECK_EXCEPTION(action)                                                                                                                                \
  if (!ForkInstanceState::get().current_info().get().thrown_exception.is_null()) [[unlikely]] {                                                                \
    action;                                                                                                                                                    \
  }

#ifdef __clang__
#define TRY_CALL_RET_(x) x
#else
#define TRY_CALL_RET_(x) std::move(x)
#endif

// ================================================================================================

#define TRY_CALL_(CallT, call, action)                                                                                                                         \
  ({                                                                                                                                                           \
    CallT x_tmp___ = (call);                                                                                                                                   \
    CHECK_EXCEPTION(action);                                                                                                                                   \
    TRY_CALL_RET_(x_tmp___);                                                                                                                                   \
  })

#define TRY_CALL_VOID_(call, action)                                                                                                                           \
  ({                                                                                                                                                           \
    (call);                                                                                                                                                    \
    CHECK_EXCEPTION(action);                                                                                                                                   \
    void();                                                                                                                                                    \
  })

// ================================================================================================

#define TRY_CALL(CallT, ResT, call) TRY_CALL_(CallT, call, return (ResT()))
#define TRY_CALL_VOID(ResT, call) TRY_CALL_VOID_(call, return (ResT()))

// ================================================================================================

#define TRY_CALL_CORO(CallT, ResT, call) TRY_CALL_(CallT, call, co_return (ResT()))
#define TRY_CALL_VOID_CORO(ResT, call) TRY_CALL_VOID_(call, co_return (ResT()))

// ================================================================================================

#define TRY_CALL_EXIT(CallT, message, call) TRY_CALL_(CallT, call, php_critical_error(message))
#define TRY_CALL_VOID_EXIT(message, call) TRY_CALL_VOID_(call, php_critical_error(message))

// ================================================================================================

namespace kphp::exception {

template<std::derived_from<C$Throwable> T>
class_instance<T> make_throwable(const string& file, int64_t line, int64_t code, const string& desc) noexcept {
  auto instance{make_instance<T>()};

  auto* instance_ptr{instance.get()};
  instance_ptr->$file = file;
  instance_ptr->$line = line;
  instance_ptr->$code = code;
  instance_ptr->$message = desc;
  return instance;
}

} // namespace kphp::exception

// ================================================================================================

template<typename T>
T f$_exception_set_location(const T& e, const string& file, int64_t line) noexcept {
  e->$file = file;
  e->$line = line;
  return e;
}

inline Exception f$err(const string& file, int64_t line, const string& code, const string& desc = {}) noexcept {
  return kphp::exception::make_throwable<C$Exception>(file, line, 0, (RuntimeContext::get().static_SB.clean() << "ERR_" << code << ": " << desc).str());
}
