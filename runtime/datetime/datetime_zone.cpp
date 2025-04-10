// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime/datetime/datetime_zone.h"

#include <cstring>

#include "runtime/datetime/timelib_wrapper.h"
#include "runtime/exception.h"

class_instance<C$DateTimeZone> f$DateTimeZone$$__construct(const class_instance<C$DateTimeZone>& self, const string& timezone) noexcept {
  if (strcmp(PHP_TIMELIB_TZ_MOSCOW, timezone.c_str()) != 0 && strcmp(PHP_TIMELIB_TZ_GMT3, timezone.c_str()) != 0) {
    string error_msg{"DateTimeZone::__construct(): Unknown or bad timezone "};
    error_msg.append(1, '(').append(timezone).append(1, ')');
    THROW_EXCEPTION(new_Exception(string{__FILE__}, __LINE__, error_msg));
    return {};
  }
  self->timezone = timezone;
  return self;
}

string f$DateTimeZone$$getName(const class_instance<C$DateTimeZone>& self) noexcept {
  return self->timezone;
}
