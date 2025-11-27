//  Compiler for PHP (aka KPHP)
//  Copyright (c) 2024 LLC «V Kontakte»
//  Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <concepts>
#include <cstdint>
#include <source_location>
#include <utility>

#include "runtime-common/core/runtime-core.h"
#include "runtime-light/stdlib/diagnostics/exception-types.h"
#include "runtime-light/stdlib/diagnostics/logs.h" // it's actually used in macros
#include "runtime-light/stdlib/fork/fork-state.h"  // it's actually used in macros

#define THROW_EXCEPTION(e)                                                                                                                                     \
  {                                                                                                                                                            \
    Throwable x_tmp___ = e;                                                                                                                                    \
    kphp::log::assertion(std::exchange(ForkInstanceState::get().current_info().get().thrown_exception, std::move(x_tmp___)).is_null());                        \
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

#define TRY_CALL_EXIT(CallT, message, call) TRY_CALL_(CallT, call, kphp::log::fatal(message))
#define TRY_CALL_VOID_EXIT(message, call) TRY_CALL_VOID_(call, kphp::log::fatal(message))

// ================================================================================================

namespace kphp::exception {

template<std::derived_from<C$Throwable> T>
class_instance<T> make_throwable(string file, int64_t line, int64_t code, string desc) noexcept {
  auto instance{make_instance<T>()};

  auto* instance_ptr{instance.get()};
  instance_ptr->$file = std::move(file);
  instance_ptr->$line = line;
  instance_ptr->$code = code;
  instance_ptr->$message = std::move(desc);
  return instance;
}

template<std::derived_from<C$Throwable> T>
class_instance<T> make_throwable(string err_msg, int64_t code = 0, std::source_location loc = std::source_location::current()) noexcept {
  return make_throwable<T>(string{loc.file_name()}, loc.line(), code, std::move(err_msg));
}

} // namespace kphp::exception

// ================================================================================================

template<typename T>
T f$_exception_set_location(const T& e, const string& file, int64_t line) noexcept {
  e->$file = file;
  e->$line = line;
  return e;
}

inline Exception f$err(string file, int64_t line, const string& code, const string& desc = {}) noexcept {
  return kphp::exception::make_throwable<C$Exception>(std::move(file), line, 0,
                                                      (RuntimeContext::get().static_SB.clean() << "ERR_" << code << ": " << desc).str());
}
