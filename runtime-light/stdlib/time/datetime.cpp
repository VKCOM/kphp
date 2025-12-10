// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime-light/stdlib/time/datetime.h"

#include "runtime-light/stdlib/diagnostics/exception-functions.h"
#include "runtime-light/stdlib/time/timelib-functions.h"

class_instance<C$DateTime> f$DateTime$$__construct(const class_instance<C$DateTime>& self, const string& datetime,
                                                   const class_instance<C$DateTimeZone>& timezone) noexcept {
  const auto& str_to_parse{datetime.empty() ? StringLibConstants::get().NOW_STR : datetime};
  auto expected_time{kphp::timelib::construct_time(std::string_view{str_to_parse.c_str(), str_to_parse.size()})};
  if (!expected_time.has_value()) [[unlikely]] {
    string err_msg;
    format_to(std::back_inserter(err_msg), "DateTime::__construct(): failed to parse datetime ({}): {}", datetime.c_str(), expected_time.error());
    THROW_EXCEPTION(kphp::exception::make_throwable<C$Exception>(err_msg));
  }

  kphp::timelib::time_t time{std::move(*expected_time)};
  timelib_tzinfo* tzi{!timezone.is_null() ? timezone->tzi : nullptr};
  if (tzi == nullptr) {
    if (time->tz_info != nullptr) {
      tzi = time->tz_info;
    } else if (auto* default_tzi{kphp::timelib::get_timezone_info(TimeInstanceState::get().default_timezone.c_str())}; default_tzi != nullptr) {
      tzi = default_tzi;
    }
  }

  kphp::timelib::fill_holes_with_now(*time, tzi);

  self->time = std::move(time);
  return self;
}

class_instance<C$DateTime> f$DateTime$$add(const class_instance<C$DateTime>& self, const class_instance<C$DateInterval>& interval) noexcept {
  self->time = kphp::timelib::add(*self->time, *interval->rel_time);
  return self;
}

class_instance<C$DateTime> f$DateTime$$createFromFormat(const string& format, const string& datetime, const class_instance<C$DateTimeZone>& timezone) noexcept {
  auto expected_time{kphp::timelib::construct_time({datetime.c_str(), datetime.size()}, format.c_str())};
  if (!expected_time.has_value()) [[unlikely]] {
    return {};
  }
  auto time{std::move(*expected_time)};
  timelib_tzinfo* tzi{!timezone.is_null() ? timezone->tzi : nullptr};
  if (tzi == nullptr) {
    if (time->tz_info != nullptr) {
      tzi = time->tz_info;
    } else if (auto* default_tzi{kphp::timelib::get_timezone_info(TimeInstanceState::get().default_timezone.c_str())}; default_tzi != nullptr) {
      tzi = default_tzi;
    }
  }

  kphp::timelib::fill_holes_with_now<true>(*time, tzi);

  class_instance<C$DateTime> date_time;
  date_time.alloc();
  date_time->time = std::move(time);
  return date_time;
}

class_instance<C$DateTime> f$DateTime$$createFromImmutable(const class_instance<C$DateTimeImmutable>& object) noexcept {
  class_instance<C$DateTime> clone;
  clone.alloc();
  clone->time = kphp::timelib::clone(*object->time);
  return clone;
}

class_instance<C$DateTime> f$DateTime$$modify(const class_instance<C$DateTime>& self, const string& modifier) noexcept {
  auto expected_success{kphp::timelib::modify(*self->time, {modifier.c_str(), modifier.size()})};
  if (!expected_success.has_value()) [[unlikely]] {
    kphp::log::warning("DateTime::modify(): failed to parse modifier ({}): {}", modifier.c_str(), expected_success.error());
    return {};
  }
  return self;
}

class_instance<C$DateTime> f$DateTime$$setDate(const class_instance<C$DateTime>& self, int64_t year, int64_t month, int64_t day) noexcept {
  kphp::timelib::set_date(*self->time, year, month, day);
  return self;
}

class_instance<C$DateTime> f$DateTime$$setTime(const class_instance<C$DateTime>& self, int64_t hour, int64_t minute, int64_t second,
                                               int64_t microsecond) noexcept {
  kphp::timelib::set_time(*self->time, hour, minute, second, microsecond);
  return self;
}

class_instance<C$DateTime> f$DateTime$$setTimestamp(const class_instance<C$DateTime>& self, int64_t timestamp) noexcept {
  kphp::timelib::set_timestamp(*self->time, timestamp);
  return self;
}

class_instance<C$DateInterval> f$DateTime$$diff(const class_instance<C$DateTime>& self, const class_instance<C$DateTimeInterface>& target_object,
                                                bool absolute) noexcept {
  class_instance<C$DateInterval> interval;
  interval.alloc();
  interval->rel_time = kphp::timelib::diff(*self->time, *target_object.get()->time, absolute);
  return interval;
}

string f$DateTime$$format(const class_instance<C$DateTime>& self, const string& format) noexcept {
  string str;
  kphp::timelib::format_to(std::back_inserter(str), {format.c_str(), format.size()}, *self->time);
  return str;
}

int64_t f$DateTime$$getTimestamp(const class_instance<C$DateTime>& self) noexcept {
  return kphp::timelib::get_timestamp(*self->time);
}
