#include "runtime/exception.h"

#include "runtime/critical_section.h"
#include "runtime/string_functions.h"

array<array<string>> f$debug_backtrace() {
  dl::enter_critical_section();//OK
  void *buffer[64];
  int nptrs = fast_backtrace(buffer, 64);
  dl::leave_critical_section();

  const string function_key("function", 8);

  array<array<string>> res(array_size(nptrs - 4, 0, true));
  char buf[20];
  for (int i = 1; i < nptrs; i++) {
    array<string> current(array_size(0, 1, false));
    dl::enter_critical_section();//OK
    snprintf(buf, 19, "%p", buffer[i]);
    dl::leave_critical_section();
    current.set_value(function_key, string(buf, (dl::size_type)strlen(buf)));
    res.push_back(current);
  }

  return res;
}

Exception CurException;

string f$Exception$$getMessage(const Exception &e) {
  return e->message;
}

int f$Exception$$getCode(const Exception &e) {
  return e->code;
}

string f$Exception$$getFile(const Exception &e) {
  return e->file;
}

int f$Exception$$getLine(const Exception &e) {
  return e->line;
}

array<array<string>> f$Exception$$getTrace(const Exception &e) {
  return e->trace;
}

Exception f$Exception$$__construct(const Exception &v$this, const string &file, int line, const string &message, int code) {
  v$this->file = file;
  v$this->line = line;
  v$this->message = message;
  v$this->code = code;
  v$this->trace = f$debug_backtrace();
  return v$this;
}

Exception new_Exception(const string &file, int line, const string &message, int code) {
  return f$Exception$$__construct(Exception().alloc(), file, line, message, code);
}


Exception f$err(const string &file, int line, const string &code, const string &desc) {
  return new_Exception(file, line, (static_SB.clean() << "ERR_" << code << ": " << desc).str(), 0);
}


string f$Exception$$getTraceAsString(const Exception &e) {
  static_SB.clean();
  for (int i = 0; i < e->trace.count(); i++) {
    array<string> current = e->trace.get_value(i);
    static_SB << '#' << i << ' ' << current.get_value(string("file", 4)) << ": " << current.get_value(string("function", 8)) << "\n";
  }
  return static_SB.str();
}

void free_exception_lib() {
  hard_reset_var(CurException);
}

