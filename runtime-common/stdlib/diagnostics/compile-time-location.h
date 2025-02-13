// Compiler for PHP (aka KPHP)
// Copyright (c) 2025 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cstdint>

#include "runtime-common/core/class-instance/refcountable-php-classes.h"
#include "runtime-common/core/runtime-core.h"
#include "runtime-common/stdlib/visitors/dummy-visitor-methods.h"

struct C$CompileTimeLocation : public refcountable_php_classes<C$CompileTimeLocation>, private DummyVisitorMethods {
  using DummyVisitorMethods::accept;

  string $file;
  string $function;
  int64_t $line;

  ~C$CompileTimeLocation() noexcept = default;

  const char *get_class() const noexcept {
    return "CompileTimeLocation";
  }
};

using CompileTimeLocation = class_instance<C$CompileTimeLocation>;

inline CompileTimeLocation f$CompileTimeLocation$$__construct(const CompileTimeLocation &v$this, const string &file, const string &function,
                                                              int64_t line) noexcept {
  v$this->$file = file;
  v$this->$function = function;
  v$this->$line = line;
  return v$this;
}

inline CompileTimeLocation f$CompileTimeLocation$$calculate(const CompileTimeLocation &v$passed) noexcept {
  return v$passed;
}
