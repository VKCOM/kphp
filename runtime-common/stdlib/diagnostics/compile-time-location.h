// Compiler for PHP (aka KPHP)
// Copyright (c) 2025 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cstdint>
#include <string_view>

#include "common/algorithms/hashes.h"
#include "runtime-common/core/class-instance/refcountable-php-classes.h"
#include "runtime-common/core/runtime-core.h"
#include "runtime-common/stdlib/visitors/dummy-visitor-methods.h"

struct C$CompileTimeLocation : public refcountable_polymorphic_php_classes_virt<>, private DummyVisitorMethods {
  using DummyVisitorMethods::accept;

  string $file;
  string $function;
  int64_t $line;

  ~C$CompileTimeLocation() override = default;

  virtual const char *get_class() const noexcept {
    return "CompileTimeLocation";
  }

  virtual int32_t get_hash() const noexcept {
    std::string_view name_view{C$CompileTimeLocation::get_class()};
    return static_cast<int32_t>(vk::murmur_hash<uint32_t>(name_view.data(), name_view.size()));
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
