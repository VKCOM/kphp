// Compiler for PHP (aka KPHP)
// Copyright (c) 2022 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "kphp-core/class-instance/refcountable-php-classes.h"
#include "kphp-core/kphp_core.h"
#include "runtime/dummy-visitor-methods.h"

struct C$DateTimeZone : public refcountable_php_classes<C$DateTimeZone>, private DummyVisitorMethods {
  using DummyVisitorMethods::accept;

  string timezone;

  const char *get_class() const noexcept {
    return R"(DateTimeZone)";
  }

  int get_hash() const noexcept {
    return 219249843;
  }
};

class_instance<C$DateTimeZone> f$DateTimeZone$$__construct(const class_instance<C$DateTimeZone> &self, const string &timezone) noexcept;
string f$DateTimeZone$$getName(const class_instance<C$DateTimeZone> &self) noexcept;
