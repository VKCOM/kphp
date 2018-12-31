#include "runtime/exception.h"

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
/*
  dl::enter_critical_section();//NOT OK
  void *buffer[64];
  int nptrs = fast_backtrace (buffer, 64);
  char **names = backtrace_symbols (buffer, nptrs);
  php_assert (names != nullptr);

  const string file_key ("file", 4);
  const string function_key ("function", 8);

  array <array <string> > res (array_size (nptrs - 1, 0, true));
  for (int i = 1; i < nptrs; i++) {
    array <string> current (array_size (0, 2, false));
    char *name_pos = strpbrk (names[i], "( ");
    php_assert (name_pos != nullptr);

    current.set_value (file_key, string (names[i], name_pos - names[i]));

    if (*name_pos == '(') {
      ++name_pos;
      char *end = strchr (name_pos, '+');
      if (end == nullptr) {
        current.set_value (function_key, string());
      } else {
        string mangled_name (name_pos, (dl::size_type)(end - name_pos));

        int status;
        char *real_name = abi::__cxa_demangle (mangled_name.c_str(), nullptr, nullptr, &status);
        if (status < 0) {
          current.set_value (function_key, mangled_name);
        } else {
          current.set_value (function_key, string (real_name, strlen (real_name)));
          free (real_name);
        }
      }
    }

    string &func = current[function_key];
    if (func[0] == 'f' && func[1] == '$') {
      const char *s = static_cast <const char *> (memchr (static_cast <const void *> (func.c_str() + 2), '(', func.size() - 2));
      if (s != nullptr) {
        func = func.substr (2, s - func.c_str() - 2);
        if (func[0] == '_' && func[1] == 't' && func[2] == '_' && func[3] == 's' && func[4] == 'r' && func[5] == 'c' && func[6] == '_' && (int)func.size() > 18) {
          func = func.substr (7, func.size() - 18);
          func.append (".php", 4);
        }

        res.push_back (current);
      }
    }
  }

  free (names);
  dl::leave_critical_section();
  return res;
*/
}


#ifdef FAST_EXCEPTIONS
Exception *CurException;
#endif

Exception &Exception::operator=(bool value) {
  bool_value = value;
  return *this;
}

Exception::Exception() :
  bool_value(false),
  code(-1),
  line(-1) {}

Exception::Exception(bool value) :
  bool_value(value),
  code(-1),
  line(-1) {}


Exception::Exception(const string &file, int line, const string &message, int code) :
  bool_value(true),
  message(message),
  code(code),
  file(file),
  line(line),
  trace(f$debug_backtrace()) {
}

string f$Exception$$getMessage(const Exception &e) {
  return e.message;
}

int f$Exception$$getCode(const Exception &e) {
  return e.code;
}

string f$Exception$$getFile(const Exception &e) {
  return e.file;
}

int f$Exception$$getLine(const Exception &e) {
  return e.line;
}

array<array<string>> f$Exception$$getTrace(const Exception &e) {
  return e.trace;
}

Exception f$Exception$$__construct(const string &file, int line, const string &message, int code) {
  return Exception(file, line, message, code);
}

Exception f$err(const string &file, int line, const string &code, const string &desc) {
  return Exception(file, line, (static_SB.clean() << "ERR_" << code << ": " << desc).str(), 0);
}


bool f$boolval(const Exception &my_exception) {
  return f$boolval(my_exception.bool_value);
}

bool eq2(const Exception &my_exception, bool value) {
  return my_exception.bool_value == value;
}

bool eq2(bool value, const Exception &my_exception) {
  return value == my_exception.bool_value;
}

bool equals(bool value, const Exception &my_exception) {
  return equals(value, my_exception.bool_value);
}

bool equals(const Exception &my_exception, bool value) {
  return equals(my_exception.bool_value, value);
}


string f$Exception$$getTraceAsString(const Exception &e) {
  static_SB.clean();
  for (int i = 0; i < e.trace.count(); i++) {
    array<string> current = e.trace.get_value(i);
    static_SB << '#' << i << ' ' << current.get_value(string("file", 4)) << ": " << current.get_value(string("function", 8)) << "\n";
  }
  return static_SB.str();
}


void exception_init_static() {
#ifdef FAST_EXCEPTIONS
  CurException = nullptr;
#endif
}

