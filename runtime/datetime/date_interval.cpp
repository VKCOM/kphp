// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime/datetime/date_interval.h"

#include "runtime/exception.h"

C$DateInterval::~C$DateInterval() {
  php_timelib_date_interval_remove(rel_time);
  rel_time = nullptr;
}

class_instance<C$DateInterval> f$DateInterval$$__construct(const class_instance<C$DateInterval> &self, const string &duration) noexcept {
  auto [rel_time, error_msg] = php_timelib_date_interval_initialize(duration);
  if (!rel_time) {
    string error{"DateInterval::__construct(): "};
    error.append(error_msg);
    THROW_EXCEPTION(new_Exception(string{__FILE__}, __LINE__, error));
    return {};
  }
  self->rel_time = rel_time;
  return self;
}

class_instance<C$DateInterval> f$DateInterval$$createFromDateString(const string &datetime) noexcept {
  auto [rel_time, error_msg] = php_timelib_date_interval_create_from_date_string(datetime);
  if (!rel_time) {
    php_warning("DateInterval::createFromDateString(): %s", error_msg.c_str());
    return {};
  }
  class_instance<C$DateInterval> date_interval;
  date_interval.alloc();
  date_interval->rel_time = rel_time;
  return date_interval;
}

string f$DateInterval$$format(const class_instance<C$DateInterval> &self, const string &format) noexcept {
  return php_timelib_date_interval_format(format, self->rel_time);
}
