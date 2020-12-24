// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "runtime/kphp_core.h"
#include "runtime/memory_usage.h"
#include "runtime/refcountable_php_classes.h"
#include "common/algorithms/hashes.h"
#include "common/wrappers/string_view.h"

array<array<string>> f$debug_backtrace();

struct C$Throwable : public refcountable_polymorphic_php_classes_virt<> {
  virtual const char *get_class() const noexcept {
    return "Throwable";
  }

  virtual int32_t get_hash() const noexcept {
    return static_cast<int32_t>(vk::std_hash(vk::string_view(get_class())));
  }

  virtual void accept(InstanceMemoryEstimateVisitor &visitor) {
    visitor("", $message);
    visitor("", $file);
    visitor("", raw_trace);
    visitor("", trace);
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
  static int classname_hash;

  virtual ~C$Exception() = default;

  const char *get_class() const noexcept override { return "Exception"; }
  int get_hash() const noexcept override { return classname_hash; }
};

struct C$Error : public C$Throwable {
  virtual ~C$Error() = default;

  const char *get_class() const noexcept override { return "Error"; }
};

using Throwable = class_instance<C$Throwable>;
using Exception = class_instance<C$Exception>;
using Error = class_instance<C$Error>;

extern Throwable CurException;

#define THROW_EXCEPTION(e) {Throwable x_tmp___ = e; php_assert(CurException.is_null()); CurException = std::move(x_tmp___);}

#define CHECK_EXCEPTION(action) if (!CurException.is_null()) {action;}

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

string exception_trace_as_string(const Throwable &e);
void exception_initialize(const Throwable &e, const string &message, int64_t code);

Exception f$Exception$$__construct(const Exception &v$this, const string &message = string(), int64_t code = 0);
Error f$Error$$__construct(const Error &v$this, const string &message = string(), int64_t code = 0);

template<typename T>
T __exception_set_location(const T &e, const string &file, int64_t line) {
  e->$file = file;
  e->$line = line;
  return e;
}

Exception new_Exception(const string &file, int64_t line, const string &message = string(), int64_t code = 0);

Exception f$err(const string &file, int64_t line, const string &code, const string &desc = string());


string f$Exception$$getMessage(const Exception &e);
string f$Error$$getMessage(const Error &e);

int64_t f$Exception$$getCode(const Exception &e);
int64_t f$Error$$getCode(const Error &e);

string f$Exception$$getFile(const Exception &e);
string f$Error$$getFile(const Error &e);

int64_t f$Exception$$getLine(const Exception &e);
int64_t f$Error$$getLine(const Error &e);

array<array<string>> f$Exception$$getTrace(const Exception &e);
array<array<string>> f$Error$$getTrace(const Error &e);

string f$Exception$$getTraceAsString(const Exception &e);
string f$Error$$getTraceAsString(const Error &e);

void free_exception_lib();

struct C$ArithmeticError : public C$Error {
  const char *get_class() const noexcept override { return "ArithmeticError"; }
};

struct C$DivisionByZeroError : public C$ArithmeticError {
  const char *get_class() const noexcept override { return "DivisionByZeroError"; }
};

struct C$AssertionError : public C$Error {
  const char *get_class() const noexcept override { return "AssertionError"; }
};

struct C$CompileError : public C$Error {
  const char *get_class() const noexcept override { return "CompileError"; }
};

struct C$ParseError : public C$CompileError {
  const char *get_class() const noexcept override { return "ParseError"; }
};

struct C$TypeError : public C$Error {
  const char *get_class() const noexcept override { return "TypeError"; }
};

struct C$ArgumentCountError : public C$TypeError {
  const char *get_class() const noexcept override { return "ArgumentCountError"; }
};

struct C$ValueError : public C$Error {
  const char *get_class() const noexcept override { return "ValueError"; }
};

struct C$UnhandledMatchError : public C$Error {
  const char *get_class() const noexcept override { return "UnhandledMatchError"; }
};

struct C$LogicException : public C$Exception {
  static int classname_hash;
  const char *get_class() const noexcept override { return "LogicException"; }
  int get_hash() const noexcept override { return classname_hash; }
};

struct C$BadFunctionCallException : public C$LogicException {
  static int classname_hash;
  const char *get_class() const noexcept override { return "BadFunctionCallException"; }
  int get_hash() const noexcept override { return classname_hash; }
};

struct C$BadMethodCallException : public C$BadFunctionCallException {
  static int classname_hash;
  const char *get_class() const noexcept override { return "BadMethodCallException"; }
  int get_hash() const noexcept override { return classname_hash; }
};

struct C$DomainException : public C$LogicException {
  static int classname_hash;
  const char *get_class() const noexcept override { return "DomainException"; }
  int get_hash() const noexcept override { return classname_hash; }
};

struct C$InvalidArgumentException : public C$LogicException {
  static int classname_hash;
  const char *get_class() const noexcept override { return "InvalidArgumentException"; }
  int get_hash() const noexcept override { return classname_hash; }
};

struct C$LengthException : public C$LogicException {
  static int classname_hash;
  const char *get_class() const noexcept override { return "LengthException"; }
  int get_hash() const noexcept override { return classname_hash; }
};

struct C$OutOfRangeException : public C$LogicException {
  static int classname_hash;
  const char *get_class() const noexcept override { return "OutOfRangeException"; }
  int get_hash() const noexcept override { return classname_hash; }
};

struct C$RuntimeException : public C$Exception {
  static int classname_hash;
  const char *get_class() const noexcept override { return "RuntimeException"; }
  int get_hash() const noexcept override { return classname_hash; }
};

struct C$OutOfBoundsException : public C$RuntimeException {
  static int classname_hash;
  const char *get_class() const noexcept override { return "OutOfBoundsException"; }
  int get_hash() const noexcept override { return classname_hash; }
};

struct C$OverflowException : public C$RuntimeException {
  static int classname_hash;
  const char *get_class() const noexcept override { return "OverflowException"; }
  int get_hash() const noexcept override { return classname_hash; }
};

struct C$RangeException : public C$RuntimeException {
  static int classname_hash;
  const char *get_class() const noexcept override { return "RangeException"; }
  int get_hash() const noexcept override { return classname_hash; }
};

struct C$UnderflowException : public C$RuntimeException {
  static int classname_hash;
  const char *get_class() const noexcept override { return "UnderflowException"; }
  int get_hash() const noexcept override { return classname_hash; }
};

struct C$UnexpectedValueException : public C$RuntimeException {
  static int classname_hash;
  const char *get_class() const noexcept override { return "UnexpectedValueException"; }
  int get_hash() const noexcept override { return classname_hash; }
};
