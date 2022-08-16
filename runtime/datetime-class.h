// Compiler for PHP (aka KPHP)
// Copyright (c) 2022 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "runtime/kphp_core.h"
#include "runtime/refcountable_php_classes.h"
#include "runtime/timelib_wrapper.h"

struct C$DateTimeZone : public refcountable_php_classes<C$DateTimeZone> {
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

struct C$DateTime : public refcountable_php_classes<C$DateTime> {
  timelib_time *time{nullptr};

  const char *get_class() const noexcept {
    return R"(DateTime)";
  }

  int get_hash() const noexcept {
    return 2141635158;
  }

  ~C$DateTime();
};

extern const string NOW;
class_instance<C$DateTime> f$DateTime$$__construct(const class_instance<C$DateTime> &self, const string &datetime = NOW,
                                                   const class_instance<C$DateTimeZone> &timezone = Optional<bool>{}) noexcept;

Optional<array<mixed>> f$DateTime$$getLastErrors() noexcept;
