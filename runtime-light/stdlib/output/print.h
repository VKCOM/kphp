// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "runtime-core/runtime-core.h"

string f$print_r(const mixed &v, bool buffered = false) noexcept;

template<class T>
string f$print_r(const class_instance<T> &v, bool buffered = false) noexcept {
  php_warning("print_r used on object");
  return f$print_r(string(v.get_class(), static_cast<string::size_type>(strlen(v.get_class()))), buffered);
}

string f$var_export(const mixed &v, bool buffered = false) noexcept;

template<class T>
string f$var_export(const class_instance<T> &v, bool buffered = false) noexcept {
  php_warning("print_r used on object");
  return f$var_export(string(v.get_class(), static_cast<string::size_type>(strlen(v.get_class()))), buffered);
}

void f$var_dump(const mixed &v) noexcept;

template<class T>
void f$var_dump(const class_instance<T> &v) noexcept {
  php_warning("print_r used on object");
  return f$var_dump(string(v.get_class(), static_cast<string::size_type>(strlen(v.get_class()))));
}
