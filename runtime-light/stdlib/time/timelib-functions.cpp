// Compiler for PHP (aka KPHP)
// Copyright (c) 2025 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime-light/stdlib/time/timelib-functions.h"

#include <cstdint>
#include <memory>
#include <optional>
#include <string_view>

#include "kphp/timelib/timelib.h"

#include "common/containers/final_action.h"
#include "runtime-common/core/allocator/platform-malloc-interface.h"
#include "runtime-light/stdlib/diagnostics/logs.h"
#include "runtime-light/stdlib/time/time-state.h"

namespace kphp::timelib {

namespace {

void patch_time(timelib_time& time, std::optional<int64_t> hour, std::optional<int64_t> minute, std::optional<int64_t> second, std::optional<int64_t> month,
                std::optional<int64_t> day, std::optional<int64_t> year) noexcept {
  hour.transform([&time](auto value) {
    time.h = value;
    return 0;
  });

  minute.transform([&time](auto value) {
    time.i = value;
    return 0;
  });

  second.transform([&time](auto value) {
    time.s = value;
    return 0;
  });

  month.transform([&time](auto value) {
    time.m = value;
    return 0;
  });

  day.transform([&time](auto value) {
    time.d = value;
    return 0;
  });

  year.transform([&time](auto value) {
    // Copied from https://github.com/php/php-src/blob/eafbc6b3e6b59786601420dfb27c3682d0cfd86c/ext/date/php_date.c#L1195
    if (value >= 0 && value < 70) {
      value += 2000;
    } else if (value >= 70 && value <= 100) {
      value += 1900;
    }
    time.y = value;
    return 0;
  });
}

} // namespace

timelib_tzinfo* get_timezone_info(const char* timezone, const timelib_tzdb* tzdb, int* errc) noexcept {
  std::string_view timezone_view{timezone};
  auto* tzinfo{TimeImageState::get().timelib_zone_cache.get(timezone_view)};
  if (tzinfo != nullptr) {
    return tzinfo;
  }
  auto& instance_timelib_zone_cache{TimeInstanceState::get().timelib_zone_cache};
  tzinfo = instance_timelib_zone_cache.get(timezone_view);
  if (tzinfo != nullptr) {
    return tzinfo;
  }

  tzinfo = (kphp::memory::libc_alloc_guard{}, timelib_parse_tzfile(timezone, tzdb, errc));
  instance_timelib_zone_cache.put(tzinfo);
  return tzinfo;
}

std::tuple<std::int64_t, std::int64_t, std::int64_t, std::int64_t, std::int64_t, std::int64_t, std::int64_t, std::int64_t, std::string_view, std::string_view>
getdate(std::int64_t timestamp, timelib_tzinfo& tzinfo) noexcept {
  auto ts = (kphp::memory::libc_alloc_guard{}, timelib_time_ctor());
  const vk::final_action ts_deleter{[ts] noexcept { (kphp::memory::libc_alloc_guard{}, timelib_time_dtor(ts)); }};
  ts->tz_info = std::addressof(tzinfo);
  ts->zone_type = TIMELIB_ZONETYPE_ID;
  (kphp::memory::libc_alloc_guard{}, timelib_unixtime2local(ts, timestamp));

  auto day_of_week = timelib_day_of_week(ts->y, ts->m, ts->d);
  return std::make_tuple(ts->s, ts->i, ts->h, ts->d, day_of_week, ts->m, ts->y, timelib_day_of_year(ts->y, ts->m, ts->d), DAY_FULL_NAMES[day_of_week],
                         MON_FULL_NAMES[ts->m - 1]);
}

int64_t gmmktime(std::optional<int64_t> hou, std::optional<int64_t> min, std::optional<int64_t> sec, std::optional<int64_t> mon, std::optional<int64_t> day,
                 std::optional<int64_t> yea) noexcept {
  auto now = (kphp::memory::libc_alloc_guard{}, timelib_time_ctor());
  const vk::final_action now_deleter{[now] noexcept { (kphp::memory::libc_alloc_guard{}, timelib_time_dtor(now)); }};
  namespace chrono = std::chrono;
  timelib_unixtime2gmt(now, chrono::time_point_cast<chrono::seconds>(chrono::system_clock::now()).time_since_epoch().count());

  patch_time(*now, hou, min, sec, mon, day, yea);

  timelib_update_ts(now, nullptr);

  return now->sse;
}

std::optional<int64_t> mktime(std::optional<int64_t> hou, std::optional<int64_t> min, std::optional<int64_t> sec, std::optional<int64_t> mon,
                              std::optional<int64_t> day, std::optional<int64_t> yea) noexcept {
  auto now = (kphp::memory::libc_alloc_guard{}, timelib_time_ctor());
  const vk::final_action now_deleter{[now] noexcept { (kphp::memory::libc_alloc_guard{}, timelib_time_dtor(now)); }};
  string default_timezone{TimeInstanceState::get().default_timezone};
  int errc{}; // it's intentionally declared as 'int' since timelib_parse_tzfile accepts 'int'
  auto* tzinfo{kphp::timelib::get_timezone_info(default_timezone.c_str(), timelib_builtin_db(), std::addressof(errc))};
  if (tzinfo == nullptr) [[unlikely]] {
    kphp::log::warning("can't get timezone info: timezone -> {}, error -> {}", default_timezone.c_str(), timelib_get_error_message(errc));
  }
  if (!tzinfo) {
    return std::nullopt;
  }
  now->tz_info = tzinfo;
  now->zone_type = TIMELIB_ZONETYPE_ID;
  namespace chrono = std::chrono;
  (kphp::memory::libc_alloc_guard{},
   timelib_unixtime2local(now, chrono::time_point_cast<chrono::seconds>(chrono::system_clock::now()).time_since_epoch().count()));

  patch_time(*now, hou, min, sec, mon, day, yea);

  (kphp::memory::libc_alloc_guard{}, timelib_update_ts(now, tzinfo));

  return now->sse;
}

std::optional<int64_t> strtotime(std::string_view timezone, std::string_view datetime, int64_t timestamp) noexcept {
  if (datetime.empty()) [[unlikely]] {
    kphp::log::warning("datetime can't be empty");
    return {};
  }

  int errc{}; // it's intentionally declared as 'int' since timelib_parse_tzfile accepts 'int'
  auto* tzinfo{kphp::timelib::get_timezone_info(timezone.data(), timelib_builtin_db(), std::addressof(errc))};
  if (tzinfo == nullptr) [[unlikely]] {
    kphp::log::warning("can't get timezone info: timezone -> {}, error -> {}", timezone, timelib_get_error_message(errc));
    return {};
  }

  kphp::memory::libc_alloc_guard _{};

  timelib_time* now{timelib_time_ctor()};
  const vk::final_action now_deleter{[now] noexcept { timelib_time_dtor(now); }};
  now->tz_info = tzinfo;
  now->zone_type = TIMELIB_ZONETYPE_ID;
  timelib_unixtime2local(now, timestamp);

  timelib_error_container* errors{};
  timelib_time* time{timelib_strtotime(datetime.data(), datetime.size(), std::addressof(errors), timelib_builtin_db(), kphp::timelib::get_timezone_info)};
  const vk::final_action time_deleter{[time] noexcept { timelib_time_dtor(time); }};
  const vk::final_action errors_deleter{[errors] noexcept { timelib_error_container_dtor(errors); }};
  if (errors->error_count != 0) [[unlikely]] {
    kphp::log::warning("got {} errors in timelib_strtotime", errors->error_count); // TODO should we logs all the errors?
    return {};
  }

  errc = 0;
  timelib_fill_holes(time, now, TIMELIB_NO_CLONE);
  timelib_update_ts(time, tzinfo);
  int64_t result_timestamp{timelib_date_to_int(time, std::addressof(errc))};
  if (errc != 0) [[unlikely]] {
    kphp::log::warning("can't convert date to int: error -> {}", timelib_get_error_message(errc));
    return {};
  }
  return result_timestamp;
}

bool valid_date(int64_t year, int64_t month, int64_t day) noexcept {
  return timelib_valid_date(year, month, day);
}

} // namespace kphp::timelib
