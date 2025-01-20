// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "runtime-common/stdlib/visitors/common-visitors-methods.h"

struct C$CompileTimeLocation : public refcountable_php_classes<C$CompileTimeLocation>, private CommonDefaultVisitorMethods {
  string $file;
  string $function;
  int64_t $line;

  ~C$CompileTimeLocation() = default;

  const char *get_class() const noexcept {
    return "CompileTimeLocation";
  }

  using CommonDefaultVisitorMethods::accept;
};

using CompileTimeLocation = class_instance<C$CompileTimeLocation>;

inline CompileTimeLocation f$CompileTimeLocation$$__construct(const CompileTimeLocation &v$this, const string &file, const string &function, int64_t line) {
  v$this->$file = file;
  v$this->$function = function;
  v$this->$line = line;
  return v$this;
}

inline CompileTimeLocation f$CompileTimeLocation$$calculate(const CompileTimeLocation &v$passed) {
  return v$passed;
}
