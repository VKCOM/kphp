// Compiler for PHP (aka KPHP)
// Copyright (c) 2025 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string_view>

#include "common/algorithms/hashes.h"
#include "runtime-common/core/class-instance/refcountable-php-classes.h"
#include "runtime-common/core/runtime-core.h"
#include "runtime-common/core/utils/kphp-assert-core.h"
#include "runtime-common/stdlib/visitors/memory-visitors.h"
#include "runtime-light/stdlib/visitors/array-visitors.h"

class InstanceDeepCopyVisitor;
class InstanceDeepDestroyVisitor;
class InstanceReferencesCountingVisitor;

// ================================================================================================

struct C$Throwable : public refcountable_polymorphic_php_classes_virt<> {
  C$Throwable() noexcept = default;
  ~C$Throwable() override = default;

  virtual const char *get_class() const noexcept {
    return "Throwable";
  }

  virtual int32_t get_hash() const noexcept {
    std::string_view name_view{get_class()};
    return static_cast<int32_t>(vk::murmur_hash<uint32_t>(name_view.data(), name_view.size()));
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

  virtual void accept(CommonMemoryEstimateVisitor &visitor) noexcept {
    generic_accept(visitor);
  }

  virtual void accept(InstanceDeepCopyVisitor & /*unused*/) noexcept {}

  virtual void accept(InstanceDeepDestroyVisitor & /*unused*/) noexcept {}

  virtual void accept(InstanceReferencesCountingVisitor & /*unused*/) noexcept {}

  virtual void accept(ToArrayVisitor &visitor) noexcept {
    generic_accept<decltype(visitor), false>(visitor); // don't process raw_trace because `mixed` can't store `void *` (to_array_debug returns array<mixed>)
  }

  string $message;
  int64_t $code{};
  string $file;
  int64_t $line{};
  array<void *> raw_trace;
  array<array<string>> trace;
};

using Throwable = class_instance<C$Throwable>;

// ================================================================================================

namespace exception_impl_ {

inline constexpr int32_t backtrace_size_limit = 64;

inline string exception_trace_as_string(const Throwable &e) noexcept {
  auto &static_SB{RuntimeContext::get().static_SB.clean()};
  for (int64_t i = 0; i < e->trace.count(); ++i) {
    array<string> current{e->trace.get_value(i)};
    static_SB << '#' << i << ' ' << current.get_value(string{"file", 4}) << ": " << current.get_value(string{"function", 8}) << "\n";
  }
  return static_SB.str();
}

inline void exception_initialize(const Throwable &e, const string &message, int64_t code) noexcept {
  e->$message = message;
  e->$code = code;
  php_warning("exception backtrace is not yet supported"); // TODO
}

} // namespace exception_impl_

// ================================================================================================

struct C$Exception : public C$Throwable {
  ~C$Exception() override = default;

  C$Exception *virtual_builtin_clone() const noexcept {
    return new C$Exception{*this};
  }

  size_t virtual_builtin_sizeof() const noexcept {
    return sizeof(*this);
  }

  const char *get_class() const noexcept override {
    return "Exception";
  }
};

using Exception = class_instance<C$Exception>;

inline Exception f$Exception$$__construct(const Exception &v$this, const string &message = {}, int64_t code = 0) noexcept {
  exception_impl_::exception_initialize(v$this, message, code);
  return v$this;
}

inline string f$Exception$$getMessage(const Exception &e) noexcept {
  return e->$message;
}

inline int64_t f$Exception$$getCode(const Exception &e) noexcept {
  return e->$code;
}

inline string f$Exception$$getFile(const Exception &e) noexcept {
  return e->$file;
}

inline int64_t f$Exception$$getLine(const Exception &e) noexcept {
  return e->$line;
}

inline array<array<string>> f$Exception$$getTrace(const Exception &e) noexcept {
  return e->trace;
}

inline string f$Exception$$getTraceAsString(const Exception &e) noexcept {
  return exception_impl_::exception_trace_as_string(e);
}

// ================================================================================================

struct C$Error : public C$Throwable {
  ~C$Error() override = default;

  C$Error *virtual_builtin_clone() const noexcept {
    return new C$Error{*this};
  }

  size_t virtual_builtin_sizeof() const noexcept {
    return sizeof(*this);
  }

  const char *get_class() const noexcept override {
    return "Error";
  }
};

using Error = class_instance<C$Error>;

inline Error f$Error$$__construct(const Error &v$this, const string &message = string(), int64_t code = 0) noexcept {
  exception_impl_::exception_initialize(v$this, message, code);
  return v$this;
}

inline string f$Error$$getMessage(const Error &e) noexcept {
  return e->$message;
}

inline int64_t f$Error$$getCode(const Error &e) noexcept {
  return e->$code;
}

inline string f$Error$$getFile(const Error &e) noexcept {
  return e->$file;
}

inline int64_t f$Error$$getLine(const Error &e) noexcept {
  return e->$line;
}

inline array<array<string>> f$Error$$getTrace(const Error &e) noexcept {
  return e->trace;
}

inline string f$Error$$getTraceAsString(const Error &e) noexcept {
  return exception_impl_::exception_trace_as_string(e);
}

// ================================================================================================

struct C$ArithmeticError : public C$Error {
  const char *get_class() const noexcept override {
    return "ArithmeticError";
  }
};

struct C$DivisionByZeroError : public C$ArithmeticError {
  const char *get_class() const noexcept override {
    return "DivisionByZeroError";
  }
};

struct C$AssertionError : public C$Error {
  const char *get_class() const noexcept override {
    return "AssertionError";
  }
};

struct C$CompileError : public C$Error {
  const char *get_class() const noexcept override {
    return "CompileError";
  }
};

struct C$ParseError : public C$CompileError {
  const char *get_class() const noexcept override {
    return "ParseError";
  }
};

struct C$TypeError : public C$Error {
  const char *get_class() const noexcept override {
    return "TypeError";
  }
};

struct C$ArgumentCountError : public C$TypeError {
  const char *get_class() const noexcept override {
    return "ArgumentCountError";
  }
};

struct C$ValueError : public C$Error {
  const char *get_class() const noexcept override {
    return "ValueError";
  }
};

struct C$UnhandledMatchError : public C$Error {
  const char *get_class() const noexcept override {
    return "UnhandledMatchError";
  }
};

struct C$LogicException : public C$Exception {
  const char *get_class() const noexcept override {
    return "LogicException";
  }
};

struct C$BadFunctionCallException : public C$LogicException {
  const char *get_class() const noexcept override {
    return "BadFunctionCallException";
  }
};

struct C$BadMethodCallException : public C$BadFunctionCallException {
  const char *get_class() const noexcept override {
    return "BadMethodCallException";
  }
};

struct C$DomainException : public C$LogicException {
  const char *get_class() const noexcept override {
    return "DomainException";
  }
};

struct C$InvalidArgumentException : public C$LogicException {
  const char *get_class() const noexcept override {
    return "InvalidArgumentException";
  }
};

struct C$LengthException : public C$LogicException {
  const char *get_class() const noexcept override {
    return "LengthException";
  }
};

struct C$OutOfRangeException : public C$LogicException {
  const char *get_class() const noexcept override {
    return "OutOfRangeException";
  }
};

struct C$RuntimeException : public C$Exception {
  const char *get_class() const noexcept override {
    return "RuntimeException";
  }
};

struct C$OutOfBoundsException : public C$RuntimeException {
  const char *get_class() const noexcept override {
    return "OutOfBoundsException";
  }
};

struct C$OverflowException : public C$RuntimeException {
  const char *get_class() const noexcept override {
    return "OverflowException";
  }
};

struct C$RangeException : public C$RuntimeException {
  const char *get_class() const noexcept override {
    return "RangeException";
  }
};

struct C$UnderflowException : public C$RuntimeException {
  const char *get_class() const noexcept override {
    return "UnderflowException";
  }
};

struct C$UnexpectedValueException : public C$RuntimeException {
  const char *get_class() const noexcept override {
    return "UnexpectedValueException";
  }
};

struct C$Random$RandomException : public C$Exception {
  const char *get_class() const noexcept override {
    return "Random\\RandomException";
  }
};
