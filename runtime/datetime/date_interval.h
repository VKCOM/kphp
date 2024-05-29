// Compiler for PHP (aka KPHP)
// Copyright (c) 2022 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "kphp-core/class-instance/refcountable-php-classes.h"
#include "runtime/datetime/timelib_wrapper.h"
#include "runtime/dummy-visitor-methods.h"

struct C$DateInterval: public refcountable_php_classes<C$DateInterval>, private DummyVisitorMethods  {
  using DummyVisitorMethods::accept;

  timelib_rel_time *rel_time{nullptr};

  const char *get_class() const  noexcept {
    return R"(DateInterval)";
  }

  int get_hash() const  noexcept {
    return -393296726;
  }

  ~C$DateInterval();
};

class_instance<C$DateInterval> f$DateInterval$$__construct(const class_instance<C$DateInterval> &self, const string &duration) noexcept;

class_instance<C$DateInterval> f$DateInterval$$createFromDateString(const string &datetime) noexcept;

string f$DateInterval$$format(const class_instance<C$DateInterval> &self, const string &format) noexcept;
