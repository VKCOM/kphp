// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime/exception.h"

#include "common/fast-backtrace.h"

#include "runtime/context/runtime-context.h"
#include "runtime/critical_section.h"
#include "runtime/string_functions.h"

namespace {

array<array<string>> make_backtrace(void** trace, int trace_size) noexcept {
  const string function_key{"function"};

  array<array<string>> res{array_size{trace_size - 4, true}};
  const size_t buf_size = 20;
  char buf[buf_size];
  for (int i = 1; i < trace_size; i++) {
    dl::enter_critical_section(); // OK
    snprintf(buf, buf_size, "%p", trace[i]);
    dl::leave_critical_section();
    array<string> current{array_size{1, false}};
    current.set_value(function_key, string{buf});
    res.emplace_back(std::move(current));
  }
  return res;
}

int get_backtrace(void** buffer, int size) noexcept {
  dl::CriticalSectionGuard critical_section;
  return fast_backtrace(buffer, size);
}

constexpr int backtrace_size_limit = 64;

} // namespace

array<array<string>> f$debug_backtrace() {
  void* buffer[backtrace_size_limit];
  const int nptrs = get_backtrace(buffer, backtrace_size_limit);
  return make_backtrace(buffer, nptrs);
}

Throwable CurException;

string f$Exception$$getMessage(const Exception& e) {
  return e->$message;
}
string f$Error$$getMessage(const Error& e) {
  return e->$message;
}

int64_t f$Exception$$getCode(const Exception& e) {
  return e->$code;
}
int64_t f$Error$$getCode(const Error& e) {
  return e->$code;
}

string f$Exception$$getFile(const Exception& e) {
  return e->$file;
}
string f$Error$$getFile(const Error& e) {
  return e->$file;
}

int64_t f$Exception$$getLine(const Exception& e) {
  return e->$line;
}
int64_t f$Error$$getLine(const Error& e) {
  return e->$line;
}

array<array<string>> f$Exception$$getTrace(const Exception& e) {
  return e->trace;
}
array<array<string>> f$Error$$getTrace(const Error& e) {
  return e->trace;
}

Exception f$Exception$$__construct(const Exception& v$this, const string& message, int64_t code) {
  exception_initialize(v$this, message, code);
  return v$this;
}

Error f$Error$$__construct(const Error& v$this, const string& message, int64_t code) {
  exception_initialize(v$this, message, code);
  return v$this;
}

Exception new_Exception(const string& file, int64_t line, const string& message, int64_t code) {
  return f$_exception_set_location(f$Exception$$__construct(Exception().alloc(), message, code), file, line);
}

Exception f$err(const string& file, int64_t line, const string& code, const string& desc) {
  return new_Exception(file, line, (kphp_runtime_context.static_SB.clean() << "ERR_" << code << ": " << desc).str(), 0);
}

string exception_trace_as_string(const Throwable& e) {
  kphp_runtime_context.static_SB.clean();
  for (int64_t i = 0; i < e->trace.count(); i++) {
    array<string> current = e->trace.get_value(i);
    kphp_runtime_context.static_SB << '#' << i << ' ' << current.get_value(string("file", 4)) << ": " << current.get_value(string("function", 8)) << "\n";
  }
  return kphp_runtime_context.static_SB.str();
}

void exception_initialize(const Throwable& e, const string& message, int64_t code) {
  e->$message = message;
  e->$code = code;

  void* buffer[backtrace_size_limit];
  const int trace_size = get_backtrace(buffer, backtrace_size_limit);
  e->raw_trace = array<void*>{array_size{trace_size, true}};
  e->raw_trace.memcpy_vector(trace_size, buffer);

  e->trace = make_backtrace(buffer, trace_size);
}

string f$Exception$$getTraceAsString(const Exception& e) {
  return exception_trace_as_string(e);
}
string f$Error$$getTraceAsString(const Error& e) {
  return exception_trace_as_string(e);
}

void free_exception_lib() {
  hard_reset_var(CurException);
}
