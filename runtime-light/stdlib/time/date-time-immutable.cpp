// Compiler for PHP (aka KPHP)
// Copyright (c) 2026 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime-light/stdlib/time/date-time-immutable.h"

#include <string_view>

#include "kphp/timelib/timelib.h"

#include "runtime-common/core/runtime-core.h"
#include "runtime-common/stdlib/time/timelib-functions.h"
#include "runtime-light/stdlib/diagnostics/exception-functions.h"
#include "runtime-light/stdlib/diagnostics/logs.h"
#include "runtime-light/stdlib/time/date-time.h"
#include "runtime-light/stdlib/time/time-state.h"
#include "runtime-light/stdlib/time/timelib-functions.h"

class_instance<C$DateTimeImmutable> f$DateTimeImmutable$$__construct(const class_instance<C$DateTimeImmutable>& self, const string& datetime,
                                                                     const class_instance<C$DateTimeZone>& timezone) noexcept {
  auto& time_instance_state{TimeInstanceState::get()};

  const auto& str_to_parse{datetime.empty() ? TimeImageState::get().NOW_STR : datetime};
  auto expected{kphp::timelib::parse_time(std::string_view{str_to_parse.c_str(), str_to_parse.size()})};
  if (!expected.has_value()) [[unlikely]] {
    static constexpr std::string_view BEFORE_DATETIME{"DateTimeImmutable::__construct(): Failed to parse time string ("};
    static constexpr std::string_view BEFORE_MESSAGE{") "};
    static constexpr size_t MIN_CAPACITY{BEFORE_DATETIME.size() + BEFORE_MESSAGE.size()};

    string err_msg;
    err_msg.reserve_at_least(MIN_CAPACITY);
    err_msg.append(BEFORE_DATETIME.data(), BEFORE_DATETIME.size())
        .append(datetime)
        .append(BEFORE_MESSAGE.data(), BEFORE_MESSAGE.size())
        .append(kphp::timelib::gen_error_msg(expected.error().get()));
    time_instance_state.update_last_errors(std::move(expected.error()));
    THROW_EXCEPTION(kphp::exception::make_throwable<C$Exception>(err_msg));
    return {};
  }
  auto& [time, errors]{*expected};
  time_instance_state.update_last_errors(std::move(errors));

  if (!timezone.is_null()) {
    kphp::timelib::fill_holes_with_now_info(time, *timezone->tzi);
  } else if (time->tz_info != nullptr) {
    kphp::timelib::fill_holes_with_now_info(time);
  } else if (auto default_tzi{kphp::timelib::get_timezone_info(time_instance_state.default_timezone.c_str())}; default_tzi.has_value()) {
    kphp::timelib::fill_holes_with_now_info(time, *default_tzi);
  } else {
    THROW_EXCEPTION(kphp::exception::make_throwable<C$Exception>(string{"DateTimeImmutable::__construct(): Failed to get default timezone"}));
    return {};
  }

  self->time = std::move(time);
  return self;
}

class_instance<C$DateTimeImmutable> f$DateTimeImmutable$$createFromFormat(const string& format, const string& datetime,
                                                                          const class_instance<C$DateTimeZone>& timezone) noexcept {
  auto& time_instance_state{TimeInstanceState::get()};

  auto expected{kphp::timelib::parse_time({datetime.c_str(), datetime.size()}, {format.c_str(), format.size()})};
  if (!expected.has_value()) [[unlikely]] {
    time_instance_state.update_last_errors(std::move(expected.error()));
    return {};
  }
  auto& [time, errors]{*expected};
  time_instance_state.update_last_errors(std::move(errors));

  if (!timezone.is_null()) {
    kphp::timelib::fill_holes_with_now_info(time, *timezone->tzi, TIMELIB_NO_CLONE | TIMELIB_OVERRIDE_TIME);
  } else if (time->tz_info != nullptr) {
    kphp::timelib::fill_holes_with_now_info(time, TIMELIB_NO_CLONE | TIMELIB_OVERRIDE_TIME);
  } else if (auto default_tzi{kphp::timelib::get_timezone_info(time_instance_state.default_timezone.c_str())}; default_tzi.has_value()) {
    kphp::timelib::fill_holes_with_now_info(time, *default_tzi, TIMELIB_NO_CLONE | TIMELIB_OVERRIDE_TIME);
  } else {
    THROW_EXCEPTION(kphp::exception::make_throwable<C$Exception>(string{"DateTimeImmutable::__construct(): Failed to get default timezone"}));
    return {};
  }

  class_instance<C$DateTimeImmutable> date_time;
  date_time.alloc();
  date_time->time = std::move(time);
  return date_time;
}

class_instance<C$DateTimeImmutable> f$DateTimeImmutable$$createFromMutable(const class_instance<C$DateTime>& object) noexcept {
  class_instance<C$DateTimeImmutable> clone;
  clone.alloc();
  clone->time = kphp::timelib::clone_time(object->time);
  return clone;
}

class_instance<C$DateTimeImmutable> f$DateTimeImmutable$$modify(const class_instance<C$DateTimeImmutable>& self, const string& modifier) noexcept {
  auto& time_instance_state{TimeInstanceState::get()};

  auto expected{kphp::timelib::parse_time({modifier.c_str(), modifier.size()}, self->time)};
  if (!expected.has_value()) [[unlikely]] {
    kphp::log::warning("DateTimeImmutable::modify(): Failed to parse time string ({}) {}", modifier.c_str(),
                       kphp::timelib::gen_error_msg(expected.error().get()).c_str());
    time_instance_state.update_last_errors(std::move(expected.error()));
    return {};
  }
  auto& [time, errors]{*expected};
  time_instance_state.update_last_errors(std::move(errors));

  auto new_date{make_instance<C$DateTimeImmutable>()};
  new_date->time = std::move(time);
  return new_date;
}
