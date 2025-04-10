// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime/datetime/datetime.h"

#include "runtime/datetime/date_interval.h"
#include "runtime/datetime/datetime_immutable.h"
#include "runtime/exception.h"

C$DateTime::~C$DateTime() {
  php_timelib_date_remove(time);
  time = nullptr;
}

const string NOW{"now", 3};

class_instance<C$DateTime> f$DateTime$$__construct(const class_instance<C$DateTime>& self, const string& datetime,
                                                   const class_instance<C$DateTimeZone>& timezone) noexcept {
  auto [time, error_msg] = php_timelib_date_initialize(timezone.is_null() ? string{} : timezone->timezone, datetime);
  if (!time) {
    string error{"DateTime::__construct(): "};
    error.append(error_msg);
    THROW_EXCEPTION(new_Exception(string{__FILE__}, __LINE__, error));
    return {};
  }
  self->time = time;
  return self;
}

class_instance<C$DateTime> f$DateTime$$add(const class_instance<C$DateTime>& self, const class_instance<C$DateInterval>& interval) noexcept {
  self->time = php_timelib_date_add(self->time, interval->rel_time);
  return self;
}

class_instance<C$DateTime> f$DateTime$$createFromFormat(const string& format, const string& datetime, const class_instance<C$DateTimeZone>& timezone) noexcept {
  auto [time, _] = php_timelib_date_initialize(timezone.is_null() ? string{} : timezone->timezone, datetime, format.c_str());
  if (!time) {
    return {};
  }
  class_instance<C$DateTime> date_time;
  date_time.alloc();
  date_time->time = time;
  return date_time;
}

class_instance<C$DateTime> f$DateTime$$createFromImmutable(const class_instance<C$DateTimeImmutable>& object) noexcept {
  class_instance<C$DateTime> clone;
  clone.alloc();
  clone->time = php_timelib_time_clone(object->time);
  return clone;
}

Optional<array<mixed>> f$DateTime$$getLastErrors() noexcept {
  return php_timelib_date_get_last_errors();
}

class_instance<C$DateTime> f$DateTime$$modify(const class_instance<C$DateTime>& self, const string& modifier) noexcept {
  auto [success, error_msg] = php_timelib_date_modify(self->time, modifier);
  if (!success) {
    php_warning("DateTime::modify(): %s", error_msg.c_str());
    return {};
  }
  return self;
}

class_instance<C$DateTime> f$DateTime$$setDate(const class_instance<C$DateTime>& self, int64_t year, int64_t month, int64_t day) noexcept {
  php_timelib_date_date_set(self->time, year, month, day);
  return self;
}

class_instance<C$DateTime> f$DateTime$$setISODate(const class_instance<C$DateTime>& self, int64_t year, int64_t week, int64_t dayOfWeek) noexcept {
  php_timelib_date_isodate_set(self->time, year, week, dayOfWeek);
  return self;
}

class_instance<C$DateTime> f$DateTime$$setTime(const class_instance<C$DateTime>& self, int64_t hour, int64_t minute, int64_t second,
                                               int64_t microsecond) noexcept {
  php_date_time_set(self->time, hour, minute, second, microsecond);
  return self;
}

class_instance<C$DateTime> f$DateTime$$setTimestamp(const class_instance<C$DateTime>& self, int64_t timestamp) noexcept {
  php_timelib_date_timestamp_set(self->time, timestamp);
  return self;
}

class_instance<C$DateTime> f$DateTime$$sub(const class_instance<C$DateTime>& self, const class_instance<C$DateInterval>& interval) noexcept {
  auto [new_time, error_msg] = php_timelib_date_sub(self->time, interval->rel_time);
  if (!new_time) {
    php_warning("DateTime::sub(): %s", error_msg.data());
    return self;
  }
  self->time = new_time;
  return self;
}

class_instance<C$DateInterval> f$DateTime$$diff(const class_instance<C$DateTime>& self, const class_instance<C$DateTimeInterface>& target_object,
                                                bool absolute) noexcept {
  class_instance<C$DateInterval> interval;
  interval.alloc();
  interval->rel_time = php_timelib_date_diff(self->time, target_object.get()->time, absolute);
  return interval;
}

string f$DateTime$$format(const class_instance<C$DateTime>& self, const string& format) noexcept {
  return php_timelib_date_format_localtime(format, self->time);
}

int64_t f$DateTime$$getOffset(const class_instance<C$DateTime>& self) noexcept {
  return php_timelib_date_offset_get(self->time);
}

int64_t f$DateTime$$getTimestamp(const class_instance<C$DateTime>& self) noexcept {
  return php_timelib_date_timestamp_get(self->time);
}
