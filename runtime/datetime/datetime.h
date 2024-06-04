// Compiler for PHP (aka KPHP)
// Copyright (c) 2022 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "runtime-core/class-instance/refcountable-php-classes.h"
#include "runtime/runtime-types.h"
#include "runtime/datetime/datetime_interface.h"
#include "runtime/datetime/datetime_zone.h"
#include "runtime/dummy-visitor-methods.h"

struct C$DateInterval;
struct C$DateTimeImmutable;

struct C$DateTime : public refcountable_polymorphic_php_classes<C$DateTimeInterface>, private DummyVisitorMethods {
  using DummyVisitorMethods::accept;

  const char *get_class() const noexcept final {
    return R"(DateTime)";
  }

  int get_hash() const noexcept final {
    return 2141635158;
  }

  ~C$DateTime();
};

extern const string NOW;
class_instance<C$DateTime> f$DateTime$$__construct(const class_instance<C$DateTime> &self, const string &datetime = NOW,
                                                   const class_instance<C$DateTimeZone> &timezone = Optional<bool>{}) noexcept;

class_instance<C$DateTime> f$DateTime$$add(const class_instance<C$DateTime> &self, const class_instance<C$DateInterval> &interval) noexcept;

class_instance<C$DateTime> f$DateTime$$createFromFormat(const string &format, const string &datetime,
                                                        const class_instance<C$DateTimeZone> &timezone = Optional<bool>{}) noexcept;

class_instance<C$DateTime> f$DateTime$$createFromImmutable(const class_instance<C$DateTimeImmutable> &object) noexcept;

Optional<array<mixed>> f$DateTime$$getLastErrors() noexcept;

class_instance<C$DateTime> f$DateTime$$modify(const class_instance<C$DateTime> &self, const string &modifier) noexcept;

class_instance<C$DateTime> f$DateTime$$setDate(const class_instance<C$DateTime> &self, int64_t year, int64_t month, int64_t day) noexcept;

class_instance<C$DateTime> f$DateTime$$setISODate(const class_instance<C$DateTime> &self, int64_t year, int64_t week, int64_t dayOfWeek = 1) noexcept;

class_instance<C$DateTime> f$DateTime$$setTime(const class_instance<C$DateTime> &self, int64_t hour, int64_t minute, int64_t second = 0,
                                               int64_t microsecond = 0) noexcept;

class_instance<C$DateTime> f$DateTime$$setTimestamp(const class_instance<C$DateTime> &self, int64_t timestamp) noexcept;

class_instance<C$DateTime> f$DateTime$$sub(const class_instance<C$DateTime> &self, const class_instance<C$DateInterval> &interval) noexcept;

class_instance<C$DateInterval> f$DateTime$$diff(const class_instance<C$DateTime> &self, const class_instance<C$DateTimeInterface> &target_object,
                                                bool absolute = false) noexcept;

string f$DateTime$$format(const class_instance<C$DateTime> &self, const string &format) noexcept;

int64_t f$DateTime$$getOffset(const class_instance<C$DateTime> &self) noexcept;

int64_t f$DateTime$$getTimestamp(const class_instance<C$DateTime> &self) noexcept;
