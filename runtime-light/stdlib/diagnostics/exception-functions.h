//  Compiler for PHP (aka KPHP)
//  Copyright (c) 2024 LLC «V Kontakte»
//  Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "common/algorithms/hashes.h"

#include "runtime-common/core/runtime-core.h"
#include "runtime-common/stdlib/array/to-array-processor.h"


struct C$Throwable : public refcountable_polymorphic_php_classes_virt<> {
  virtual const char *get_class() const noexcept {
    return "Throwable";
  }

  virtual int32_t get_hash() const noexcept {
    return static_cast<int32_t>(vk::std_hash(std::string_view(get_class())));
  }

  template<class Visitor, bool process_raw_trace = true>
  void generic_accept(Visitor &&visitor) noexcept {
    visitor("message", $message);
    visitor("code", $code);
    visitor("file", $file);
    visitor("line", $line);
    visitor("__trace", trace);
    if constexpr (process_raw_trace) {
      visitor("__raw_trace", raw_trace);
    }
  }

  virtual void accept(ToArrayVisitor &visitor) noexcept {
    generic_accept<decltype(visitor), false>(visitor); // don't process raw_trace because `mixed` can't store `void *` (to_array_debug returns array<mixed>)
  }

  C$Throwable() __attribute__((always_inline)) = default;
  ~C$Throwable() __attribute__((always_inline)) = default;

  string $message;
  int64_t $code = 0;
  string $file;
  int64_t $line = 0;
  array<void *> raw_trace;
  array<array<string>> trace;
};

struct C$Exception : public C$Throwable {
  virtual ~C$Exception() = default;

  C$Exception* virtual_builtin_clone() const noexcept {
    return new C$Exception{*this};
  }

  size_t virtual_builtin_sizeof() const noexcept {
    return sizeof(*this);
  }

  const char *get_class() const noexcept override { return "Exception"; }
};

struct C$Error : public C$Throwable {
  virtual ~C$Error() = default;

  C$Error* virtual_builtin_clone() const noexcept {
    return new C$Error{*this};
  }

  size_t virtual_builtin_sizeof() const noexcept {
    return sizeof(*this);
  }

  const char *get_class() const noexcept override { return "Error"; }
};

using Throwable = class_instance<C$Throwable>;
using Exception = class_instance<C$Exception>;
using Error = class_instance<C$Error>;


Exception f$Exception$$__construct(const Exception &v$this, const string &message = string(), int64_t code = 0);


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
T f$_exception_set_location([[maybe_unused]] const T &e, [[maybe_unused]] const string &file, [[maybe_unused]] int64_t line) {
  php_critical_error("call to unsupported function");
}
