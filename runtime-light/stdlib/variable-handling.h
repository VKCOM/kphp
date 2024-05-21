#pragma once

#include "runtime-core/runtime-core.h"
#include "runtime-light/stdlib/string-functions.h"

string f$print_r(const mixed &v, bool buffered = false);

template<class T>
string f$print_r(const class_instance<T> &v, bool buffered = false) {
  php_warning("print_r used on object");
  return f$print_r(string(v.get_class(), (string::size_type)strlen(v.get_class())), buffered);
}

string f$var_export(const mixed &v, bool buffered = false);

template<class T>
string f$var_export(const class_instance<T> &v, bool buffered = false) {
  php_warning("print_r used on object");
  return f$var_export(string(v.get_class(), (string::size_type)strlen(v.get_class())), buffered);
}

void f$var_dump(const mixed &v);

template<class T>
void f$var_dump(const class_instance<T> &v) {
  php_warning("print_r used on object");
  return f$var_dump(string(v.get_class(), (string::size_type)strlen(v.get_class())));
}

