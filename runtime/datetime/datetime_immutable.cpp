// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime/datetime/datetime_immutable.h"

#include <cstring>

#include "runtime/datetime/date_interval.h"
#include "runtime/datetime/datetime.h"
#include "runtime/datetime/datetime_functions.h"
#include "runtime/exception.h"

static class_instance<C$DateTimeImmutable> clone_immutable(const class_instance<C$DateTimeImmutable> &origin) {
  class_instance<C$DateTimeImmutable> clone;
  clone.alloc();
  clone->time = php_timelib_time_clone(origin->time);
  return clone;
}

C$DateTimeImmutable::~C$DateTimeImmutable() {
  php_timelib_date_remove(time);
  time = nullptr;
}

class_instance<C$DateTimeImmutable> f$DateTimeImmutable$$__construct(const class_instance<C$DateTimeImmutable> &self, const string &datetime,
                                                                     const class_instance<C$DateTimeZone> &timezone) noexcept {
  auto [time, error_msg] = php_timelib_date_initialize(timezone.is_null() ? string{} : timezone->timezone, datetime);
  if (!time) {
    string error{"DateTimeImmutable::__construct(): "};
    error.append(error_msg);
    THROW_EXCEPTION(new_Exception(string{__FILE__}, __LINE__, error));
    return {};
  }
  self->time = time;
  return self;
}

class_instance<C$DateTimeImmutable> f$DateTimeImmutable$$add(const class_instance<C$DateTimeImmutable> &self,
                                                             const class_instance<C$DateInterval> &interval) noexcept {
  auto new_date = clone_immutable(self);
  new_date->time = php_timelib_date_add(new_date->time, interval->rel_time);
  return new_date;
}

class_instance<C$DateTimeImmutable> f$DateTimeImmutable$$createFromFormat(const string &format, const string &datetime,
                                                                          const class_instance<C$DateTimeZone> &timezone) noexcept {
  auto [time, _] = php_timelib_date_initialize(timezone.is_null() ? string{} : timezone->timezone, datetime, format.c_str());
  if (!time) {
    return {};
  }
  class_instance<C$DateTimeImmutable> date_time;
  date_time.alloc();
  date_time->time = time;
  return date_time;
}

class_instance<C$DateTimeImmutable> f$DateTimeImmutable$$createFromMutable(const class_instance<C$DateTime> &object) noexcept {
  class_instance<C$DateTimeImmutable> clone;
  clone.alloc();
  clone->time = php_timelib_time_clone(object->time);
  return clone;
}

Optional<array<mixed>> f$DateTimeImmutable$$getLastErrors() noexcept {
  return php_timelib_date_get_last_errors();
}

class_instance<C$DateTimeImmutable> f$DateTimeImmutable$$modify(const class_instance<C$DateTimeImmutable> &self, const string &modifier) noexcept {
  auto new_date = clone_immutable(self);
  auto [success, error_msg] = php_timelib_date_modify(new_date->time, modifier);
  if (!success) {
    php_warning("DateTimeImmutable::modify(): %s", error_msg.c_str());
    return {};
  }
  return new_date;
}

class_instance<C$DateTimeImmutable> f$DateTimeImmutable$$setDate(const class_instance<C$DateTimeImmutable> &self, int64_t year, int64_t month,
                                                                 int64_t day) noexcept {
  auto new_date = clone_immutable(self);
  php_timelib_date_date_set(new_date->time, year, month, day);
  return new_date;
}

class_instance<C$DateTimeImmutable> f$DateTimeImmutable$$setISODate(const class_instance<C$DateTimeImmutable> &self, int64_t year, int64_t week,
                                                                    int64_t dayOfWeek) noexcept {
  auto new_date = clone_immutable(self);
  php_timelib_date_isodate_set(new_date->time, year, week, dayOfWeek);
  return new_date;
}

class_instance<C$DateTimeImmutable> f$DateTimeImmutable$$setTime(const class_instance<C$DateTimeImmutable> &self, int64_t hour, int64_t minute, int64_t second,
                                                                 int64_t microsecond) noexcept {
  auto new_date = clone_immutable(self);
  php_date_time_set(new_date->time, hour, minute, second, microsecond);
  return new_date;
}

class_instance<C$DateTimeImmutable> f$DateTimeImmutable$$setTimestamp(const class_instance<C$DateTimeImmutable> &self, int64_t timestamp) noexcept {
  auto new_date = clone_immutable(self);
  php_timelib_date_timestamp_set(new_date->time, timestamp);
  return new_date;
}

class_instance<C$DateTimeImmutable> f$DateTimeImmutable$$sub(const class_instance<C$DateTimeImmutable> &self,
                                                             const class_instance<C$DateInterval> &interval) noexcept {
  auto new_date = clone_immutable(self);
  auto [new_time, error_msg] = php_timelib_date_sub(new_date->time, interval->rel_time);
  if (!new_time) {
    php_warning("DateTimeImmutable::sub(): %s", error_msg.data());
    return new_date;
  }
  new_date->time = new_time;
  return new_date;
}

class_instance<C$DateInterval> f$DateTimeImmutable$$diff(const class_instance<C$DateTimeImmutable> &self,
                                                         const class_instance<C$DateTimeInterface> &target_object, bool absolute) noexcept {
  class_instance<C$DateInterval> interval;
  interval.alloc();
  interval->rel_time = php_timelib_date_diff(self->time, target_object.get()->time, absolute);
  return interval;
}

string f$DateTimeImmutable$$format(const class_instance<C$DateTimeImmutable> &self, const string &format) noexcept {
  return php_timelib_date_format_localtime(format, self->time);
}

int64_t f$DateTimeImmutable$$getOffset(const class_instance<C$DateTimeImmutable> &self) noexcept {
  return php_timelib_date_offset_get(self->time);
}

int64_t f$DateTimeImmutable$$getTimestamp(const class_instance<C$DateTimeImmutable> &self) noexcept {
  return php_timelib_date_timestamp_get(self->time);
}
