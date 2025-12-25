// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime-light/stdlib/time/datetimeimmutable.h"

#include <chrono>
#include <cstddef>
#include <cstring>
#include <format>
#include <iterator>
#include <string_view>
#include <type_traits>
#include <utility>

#include "common/containers/final_action.h"
#include "runtime-common/core/runtime-core.h"
#include "runtime-light/stdlib/diagnostics/exception-functions.h"
#include "runtime-light/stdlib/diagnostics/logs.h"
#include "runtime-light/stdlib/time/dateinterval.h"
#include "runtime-light/stdlib/time/datetime.h"
#include "runtime-light/stdlib/time/time-functions.h"
#include "runtime-light/stdlib/time/timelib-functions.h"

namespace {

class_instance<C$DateTimeImmutable> clone_immutable(const class_instance<C$DateTimeImmutable>& origin) noexcept {
  class_instance<C$DateTimeImmutable> clone;
  clone.alloc();
  clone->time = kphp::timelib::clone(*origin->time);
  return clone;
}

} // namespace

class_instance<C$DateTimeImmutable> f$DateTimeImmutable$$__construct(const class_instance<C$DateTimeImmutable>& self, const string& datetime,
                                                                     const class_instance<C$DateTimeZone>& timezone) noexcept {
  const auto& str_to_parse{datetime.empty() ? StringLibConstants::get().NOW_STR : datetime};
  auto [time, errors]{kphp::timelib::parse_time(std::string_view{str_to_parse.c_str(), str_to_parse.size()})};
  if (time == nullptr) [[unlikely]] {
    string err_msg;
    format_to(std::back_inserter(err_msg), "DateTimeImmutable::__construct(): Failed to parse time string ({}) {}", datetime.c_str(), errors);
    TimeInstanceState::get().update_last_errors(std::move(errors));
    THROW_EXCEPTION(kphp::exception::make_throwable<C$Exception>(err_msg));
    return {};
  }
  TimeInstanceState::get().update_last_errors(std::move(errors));

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

class_instance<C$DateTimeImmutable> f$DateTimeImmutable$$add(const class_instance<C$DateTimeImmutable>& self,
                                                             const class_instance<C$DateInterval>& interval) noexcept {
  auto new_date{clone_immutable(self)};
  new_date->time = kphp::timelib::add(*new_date->time, *interval->rel_time);
  return new_date;
}

class_instance<C$DateTimeImmutable> f$DateTimeImmutable$$createFromFormat(const string& format, const string& datetime,
                                                                          const class_instance<C$DateTimeZone>& timezone) noexcept {
  auto [time, errors]{kphp::timelib::parse_time({datetime.c_str(), datetime.size()}, format.c_str())};
  if (time == nullptr) [[unlikely]] {
    TimeInstanceState::get().update_last_errors(std::move(errors));
    return {};
  }
  TimeInstanceState::get().update_last_errors(std::move(errors));
  timelib_tzinfo* tzi{!timezone.is_null() ? timezone->tzi : nullptr};
  if (tzi == nullptr) {
    if (time->tz_info != nullptr) {
      tzi = time->tz_info;
    } else if (auto* default_tzi{kphp::timelib::get_timezone_info(TimeInstanceState::get().default_timezone.c_str())}; default_tzi != nullptr) {
      tzi = default_tzi;
    }
  }

  kphp::timelib::fill_holes_with_now<true>(*time, tzi);

  class_instance<C$DateTimeImmutable> date_time;
  date_time.alloc();
  date_time->time = std::move(time);
  return date_time;
}

class_instance<C$DateTimeImmutable> f$DateTimeImmutable$$createFromMutable(const class_instance<C$DateTime>& object) noexcept {
  class_instance<C$DateTimeImmutable> clone;
  clone.alloc();
  clone->time = kphp::timelib::clone(*object->time);
  return clone;
}

Optional<array<mixed>> f$DateTimeImmutable$$getLastErrors() noexcept {
  return TimeInstanceState::get().get_last_errors();
}

class_instance<C$DateTimeImmutable> f$DateTimeImmutable$$modify(const class_instance<C$DateTimeImmutable>& self, const string& modifier) noexcept {
  auto new_date{clone_immutable(self)};
  auto errors{kphp::timelib::modify(*new_date->time, {modifier.c_str(), modifier.size()})};
  if (errors != nullptr && errors->error_count > 0) [[unlikely]] {
    kphp::log::warning("DateTimeImmutable::modify(): Failed to parse time string ({}) {}", modifier.c_str(), errors);
    TimeInstanceState::get().update_last_errors(std::move(errors));
    return {};
  }
  TimeInstanceState::get().update_last_errors(std::move(errors));
  return new_date;
}

class_instance<C$DateTimeImmutable> f$DateTimeImmutable$$setDate(const class_instance<C$DateTimeImmutable>& self, int64_t year, int64_t month,
                                                                 int64_t day) noexcept {
  auto new_date{clone_immutable(self)};
  kphp::timelib::set_date(*new_date->time, year, month, day);
  return new_date;
}

class_instance<C$DateTimeImmutable> f$DateTimeImmutable$$setISODate(const class_instance<C$DateTimeImmutable>& self, int64_t year, int64_t week,
                                                                    int64_t dayOfWeek) noexcept {
  auto new_date{clone_immutable(self)};
  kphp::timelib::set_isodate(*new_date->time, year, week, dayOfWeek);
  return new_date;
}

class_instance<C$DateTimeImmutable> f$DateTimeImmutable$$setTime(const class_instance<C$DateTimeImmutable>& self, int64_t hour, int64_t minute, int64_t second,
                                                                 int64_t microsecond) noexcept {
  auto new_date{clone_immutable(self)};
  kphp::timelib::set_time(*new_date->time, hour, minute, second, microsecond);
  return new_date;
}

class_instance<C$DateTimeImmutable> f$DateTimeImmutable$$setTimestamp(const class_instance<C$DateTimeImmutable>& self, int64_t timestamp) noexcept {
  auto new_date{clone_immutable(self)};
  kphp::timelib::set_timestamp(*new_date->time, timestamp);
  return new_date;
}

class_instance<C$DateTimeImmutable> f$DateTimeImmutable$$sub(const class_instance<C$DateTimeImmutable>& self,
                                                             const class_instance<C$DateInterval>& interval) noexcept {
  auto new_date{clone_immutable(self)};
  auto expected_new_time{kphp::timelib::sub(*new_date->time, *interval->rel_time)};
  if (!expected_new_time.has_value()) {
    kphp::log::warning("DateTimeImmutable::sub(): {}", expected_new_time.error());
    return new_date;
  }
  new_date->time = std::move(*expected_new_time);
  return new_date;
}

class_instance<C$DateInterval> f$DateTimeImmutable$$diff(const class_instance<C$DateTimeImmutable>& self,
                                                         const class_instance<C$DateTimeInterface>& target_object, bool absolute) noexcept {
  class_instance<C$DateInterval> interval;
  interval.alloc();
  interval->rel_time = kphp::timelib::diff(*self->time, *target_object.get()->time, absolute);
  return interval;
}

string f$DateTimeImmutable$$format(const class_instance<C$DateTimeImmutable>& self, const string& format) noexcept {
  string str;
  kphp::timelib::format_to(std::back_inserter(str), {format.c_str(), format.size()}, *self->time);
  return str;
}

int64_t f$DateTimeImmutable$$getOffset(const class_instance<C$DateTimeImmutable>& self) noexcept {
  return kphp::timelib::get_offset(*self->time);
}

int64_t f$DateTimeImmutable$$getTimestamp(const class_instance<C$DateTimeImmutable>& self) noexcept {
  return kphp::timelib::get_timestamp(*(self->time));
}
