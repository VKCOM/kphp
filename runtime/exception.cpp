// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime/exception.h"

#include "common/fast-backtrace.h"

#include "runtime/critical_section.h"
#include "runtime/string_functions.h"

namespace {

array<array<string>> make_backtrace(void **trace, int trace_size) noexcept {
  const string function_key{"function"};

  array<array<string>> res{array_size{trace_size - 4, 0, true}};
  char buf[20];
  for (int i = 1; i < trace_size; i++) {
    dl::enter_critical_section();//OK
    snprintf(buf, 19, "%p", trace[i]);
    dl::leave_critical_section();
    array<string> current{array_size{0, 1, false}};
    current.set_value(function_key, string{buf});
    res.emplace_back(std::move(current));
  }
  return res;
}

int get_backtrace(void **buffer, int size) noexcept {
  dl::CriticalSectionGuard critical_section;
  return fast_backtrace(buffer, size);
}

constexpr int backtrace_size_limit = 64;

} // namsepace

array<array<string>> f$debug_backtrace() {
  void *buffer[backtrace_size_limit];
  const int nptrs = get_backtrace(buffer, backtrace_size_limit);
  return make_backtrace(buffer, nptrs);
}

Exception CurException;

string f$Exception$$getMessage(const Exception &e) {
  return e->message;
}

int64_t f$Exception$$getCode(const Exception &e) {
  return e->code;
}

string f$Exception$$getFile(const Exception &e) {
  return e->file;
}

int64_t f$Exception$$getLine(const Exception &e) {
  return e->line;
}

array<array<string>> f$Exception$$getTrace(const Exception &e) {
  return e->trace;
}

Exception f$Exception$$__construct(const Exception &v$this, const string &file, int64_t line, const string &message, int64_t code) {
  v$this->file = file;
  v$this->line = line;
  v$this->message = message;
  v$this->code = code;

  void *buffer[backtrace_size_limit];
  const int trace_size = get_backtrace(buffer, backtrace_size_limit);
  v$this->raw_trace = array<void *>{array_size{trace_size, 0, true}};
  v$this->raw_trace.memcpy_vector(trace_size, buffer);

  v$this->trace = make_backtrace(buffer, trace_size);
  return v$this;
}

Exception new_Exception(const string &file, int64_t line, const string &message, int64_t code) {
  return f$Exception$$__construct(Exception().alloc(), file, line, message, code);
}


Exception f$err(const string &file, int64_t line, const string &code, const string &desc) {
  return new_Exception(file, line, (static_SB.clean() << "ERR_" << code << ": " << desc).str(), 0);
}


string f$Exception$$getTraceAsString(const Exception &e) {
  static_SB.clean();
  for (int64_t i = 0; i < e->trace.count(); i++) {
    array<string> current = e->trace.get_value(i);
    static_SB << '#' << i << ' ' << current.get_value(string("file", 4)) << ": " << current.get_value(string("function", 8)) << "\n";
  }
  return static_SB.str();
}

void free_exception_lib() {
  hard_reset_var(CurException);
}

