// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime/datetime-class.h"

#include <cstring>

#include "runtime/exception.h"
#include "runtime/datetime.h"

class_instance<C$DateTimeZone> f$DateTimeZone$$__construct(const class_instance<C$DateTimeZone> &self, const string &timezone) noexcept {
  if (strcmp(PHP_TIMELIB_TZ_MOSCOW, timezone.c_str()) != 0 && strcmp(PHP_TIMELIB_TZ_GMT3, timezone.c_str()) != 0) {
    string error_msg{"DateTimeZone::__construct(): Unknown or bad timezone "};
    error_msg.append(1, '(').append(timezone).append(1, ')');
    THROW_EXCEPTION(new_Exception(string{__FILE__}, __LINE__, error_msg));
    return {};
  }
  self->timezone = timezone;
  return self;
}

string f$DateTimeZone$$getName(const class_instance<C$DateTimeZone> &self) noexcept {
  return self->timezone;
}

C$DateTime::~C$DateTime() {
  php_timelib_date_remove(time);
  time = nullptr;
}

const string NOW{"now", 3};

class_instance<C$DateTime> f$DateTime$$__construct(const class_instance<C$DateTime> &self, const string &datetime,
                                                   const class_instance<C$DateTimeZone> &timezone) noexcept {
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

Optional<array<mixed>> f$DateTime$$getLastErrors() noexcept {
  return php_timelib_date_get_last_errors();
}

class_instance<C$DateTime> f$DateTime$$modify(const class_instance<C$DateTime> &self, const string &modifier) noexcept {
  auto [success, error_msg] = php_timelib_date_modify(self->time, modifier);
  if (!success) {
    php_warning("DateTime::modify(): %s", error_msg.c_str());
    return {};
  }
  return self;
}

class_instance<C$DateTime> f$DateTime$$setDate(const class_instance<C$DateTime> &self, int64_t year, int64_t month, int64_t day) noexcept {
  php_timelib_date_date_set(self->time, year, month, day);
  return self;
}

class_instance<C$DateTime> f$DateTime$$setTimestamp(const class_instance<C$DateTime> &self, int64_t timestamp) noexcept {
  php_timelib_date_timestamp_set(self->time, timestamp);
  return self;
}

string f$DateTime$$format(const class_instance<C$DateTime> &self, const string &format) noexcept {
  return php_timelib_date_format_localtime(format, self->time);
}

int64_t f$DateTime$$getTimestamp(const class_instance<C$DateTime> &self) noexcept {
  return php_timelib_date_timestamp_get(self->time);
}
