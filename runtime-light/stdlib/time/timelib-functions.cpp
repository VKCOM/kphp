// Compiler for PHP (aka KPHP)
// Copyright (c) 2025 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime-light/stdlib/time/timelib-functions.h"

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <expected>
#include <format>
#include <memory>
#include <optional>
#include <string_view>
#include <utility>

#include "kphp/timelib/timelib.h"

#include "common/containers/final_action.h"
#include "runtime-light/stdlib/diagnostics/logs.h"
#include "runtime-light/stdlib/time/time-state.h"

namespace kphp::timelib {

error::~error() {
  if (err != nullptr) {
    destruct(err);
  }
}

std::expected<time_t, error> construct_time(std::string_view time_sv) noexcept {
  timelib_error_container* errors{};
  time_t time{(kphp::memory::libc_alloc_guard{},
                      timelib_strtotime(time_sv.data(), time_sv.size(), std::addressof(errors), timelib_builtin_db(), kphp::timelib::get_timezone_info))};
  if (errors->error_count != 0) [[unlikely]] {
    destruct(*time);
    return std::unexpected{error{errors}};
  }

  destruct(*errors);
  return time;
}

std::expected<time_t, error> construct_time(std::string_view time_sv, const char* format) noexcept {
  timelib_error_container* err{nullptr};
  time_t t{(kphp::memory::libc_alloc_guard{}, timelib_parse_from_format(format, time_sv.data(), time_sv.size(), std::addressof(err),
                                                                               timelib_builtin_db(), kphp::timelib::get_timezone_info))};

  if (err && err->error_count) {
    destruct(*t);
    return std::unexpected{error{err}};
  }
  destruct(*err);

  return t;
}

time_offset_t construct_time_offset(timelib_time* t) noexcept {
  if (t->zone_type == TIMELIB_ZONETYPE_ABBR) {
    time_offset_t offset{(kphp::memory::libc_alloc_guard{}, timelib_time_offset_ctor())};
    offset->offset = (t->z + (t->dst * 3600));
    offset->leap_secs = 0;
    offset->is_dst = t->dst;
    offset->abbr = (kphp::memory::libc_alloc_guard{}, timelib_strdup(t->tz_abbr));
    return offset;
  }
  if (t->zone_type == TIMELIB_ZONETYPE_OFFSET) {
    time_offset_t offset{(kphp::memory::libc_alloc_guard{}, timelib_time_offset_ctor())};
    offset->offset = (t->z);
    offset->leap_secs = 0;
    offset->is_dst = 0;
    offset->abbr = (kphp::memory::libc_alloc_guard{}, static_cast<char*>(timelib_malloc(9))); // GMT±xxxx\0
    // set upper bound to 99 just to ensure that 'hours_offset' fits in {:02}
    auto hours_offset{std::min(std::abs(offset->offset / 3600), 99)};
    std::format_to_n(offset->abbr, 9, "GMT{}{:02}{:02}", (offset->offset < 0) ? '-' : '+', hours_offset, std::abs((offset->offset % 3600) / 60));
    return offset;
  }
  return time_offset_t{timelib_get_time_zone_info(t->sse, t->tz_info)};
}

std::expected<rel_time_t, error> construct_interval(std::string_view format) noexcept {
  timelib_time* b{nullptr};
  timelib_time* e{nullptr};
  vk::final_action e_deleter{[e]() { kphp::memory::libc_alloc_guard{}, free(e); }};
  vk::final_action b_deleter{[b]() { kphp::memory::libc_alloc_guard{}, free(b); }};
  timelib_rel_time* p{nullptr};
  int r{};
  timelib_error_container* errors{nullptr};

  kphp::memory::libc_alloc_guard{},
      timelib_strtointerval(format.data(), format.size(), std::addressof(b), std::addressof(e), std::addressof(p), std::addressof(r), std::addressof(errors));

  if (errors->error_count > 0) {
    if (p != nullptr) {
      destruct(*p);
    }
    return std::unexpected{error{errors}};
  }
  destruct(*errors);

  if (p != nullptr) {
    return rel_time_t{p};
  }

  if (b != nullptr && e != nullptr) {
    timelib_update_ts(b, nullptr);
    timelib_update_ts(e, nullptr);
    return kphp::memory::libc_alloc_guard{}, rel_time_t{timelib_diff(b, e)};
  }

  return std::unexpected{error{nullptr}};
}

timelib_time* add(timelib_time* t, timelib_rel_time* interval) noexcept {
  timelib_time* new_time{(kphp::memory::libc_alloc_guard{}, timelib_add(t, interval))};
  kphp::memory::libc_alloc_guard{}, timelib_time_dtor(t);
  return new_time;
}

timelib_rel_time* clone(timelib_rel_time* rt) noexcept {
  return kphp::memory::libc_alloc_guard{}, timelib_rel_time_clone(rt);
}

timelib_time* clone(timelib_time* t) noexcept {
  return kphp::memory::libc_alloc_guard{}, timelib_time_clone(t);
}

timelib_rel_time* date_diff(timelib_time* time1, timelib_time* time2, bool absolute) noexcept {
  timelib_update_ts(time1, nullptr);
  timelib_update_ts(time2, nullptr);

  timelib_rel_time* diff{(kphp::memory::libc_alloc_guard{}, timelib_diff(time1, time2))};
  if (absolute) {
    diff->invert = 0;
  }
  return diff;
}

std::string_view date_full_day_name(timelib_sll y, timelib_sll m, timelib_sll d) noexcept {
  timelib_sll day_of_week = timelib_day_of_week(y, m, d);
  if (day_of_week < 0) {
    return "Unknown";
  }
  return DAY_FULL_NAMES[day_of_week];
}

std::string_view date_short_day_name(timelib_sll y, timelib_sll m, timelib_sll d) noexcept {
  timelib_sll day_of_week = timelib_day_of_week(y, m, d);
  if (day_of_week < 0) {
    return "Unknown";
  }
  return DAY_SHORT_NAMES[day_of_week];
}

int64_t date_timestamp_get(timelib_time& t) noexcept {
  timelib_update_ts(std::addressof(t), nullptr);

  int error = 0;
  timelib_long timestamp = timelib_date_to_int(std::addressof(t), &error);
  // the 'error' should be always 0 on x64 platform
  log::assertion(error == 0);

  return timestamp;
}

void date_timestamp_set(timelib_time* t, int64_t timestamp) noexcept {
  kphp::memory::libc_alloc_guard{}, timelib_unixtime2local(t, static_cast<timelib_sll>(timestamp));
  timelib_update_ts(t, nullptr);
  t->us = 0;
}

const char* english_suffix(timelib_sll number) noexcept {
  if (number >= 10 && number <= 19) {
    return "th";
  } else {
    switch (number % 10) {
    case 1:
      return "st";
    case 2:
      return "nd";
    case 3:
      return "rd";
    }
  }
  return "th";
}

timelib_tzinfo* get_timezone_info(const char* timezone) noexcept {
  int errc{}; // it's intentionally declared as 'int' since timelib_parse_tzfile accepts 'int'
  auto* tzinfo{kphp::timelib::get_timezone_info(timezone, timelib_builtin_db(), std::addressof(errc))};
  if (tzinfo == nullptr) [[unlikely]] {
    kphp::log::warning("can't get timezone info: timezone -> {}, error -> {}", timezone, timelib_get_error_message(errc));
  }
  return tzinfo;
}

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

int64_t gmmktime(std::optional<int64_t> hou, std::optional<int64_t> min, std::optional<int64_t> sec, std::optional<int64_t> mon, std::optional<int64_t> day,
                 std::optional<int64_t> yea) noexcept {
  auto* now{(kphp::memory::libc_alloc_guard{}, timelib_time_ctor())};
  const vk::final_action now_deleter{[now] noexcept { (kphp::memory::libc_alloc_guard{}, timelib_time_dtor(now)); }};
  namespace chrono = std::chrono;
  timelib_unixtime2gmt(now, chrono::time_point_cast<chrono::seconds>(chrono::system_clock::now()).time_since_epoch().count());

  hou.transform([now](auto value) noexcept {
    now->h = value;
    return 0;
  });

  min.transform([now](auto value) noexcept {
    now->i = value;
    return 0;
  });

  sec.transform([now](auto value) noexcept {
    now->s = value;
    return 0;
  });
  mon.transform([now](auto value) noexcept {
    now->m = value;
    return 0;
  });

  day.transform([now](auto value) noexcept {
    now->d = value;
    return 0;
  });

  yea.transform([now](auto value) noexcept {
    now->y = value;
    return 0;
  });

  timelib_update_ts(now, nullptr);

  return now->sse;
}

bool is_leap_year(int32_t year) noexcept {
  return (year % 4 == 0) && (year % 100 != 0 || year % 400 == 0);
}

std::optional<int64_t> mktime(std::optional<int64_t> hou, std::optional<int64_t> min, std::optional<int64_t> sec, std::optional<int64_t> mon,
                              std::optional<int64_t> day, std::optional<int64_t> yea) noexcept {
  auto* now{(kphp::memory::libc_alloc_guard{}, timelib_time_ctor())};
  const vk::final_action now_deleter{[now] noexcept { (kphp::memory::libc_alloc_guard{}, timelib_time_dtor(now)); }};
  string default_timezone{TimeInstanceState::get().default_timezone};
  int errc{}; // it's intentionally declared as 'int' since timelib_parse_tzfile accepts 'int'
  auto* tzinfo{kphp::timelib::get_timezone_info(default_timezone.c_str(), timelib_builtin_db(), std::addressof(errc))};
  if (tzinfo == nullptr) [[unlikely]] {
    kphp::log::warning("can't get timezone info: timezone -> {}, error -> {}", default_timezone.c_str(), timelib_get_error_message(errc));
    return std::nullopt;
  }
  now->tz_info = tzinfo;
  now->zone_type = TIMELIB_ZONETYPE_ID;
  namespace chrono = std::chrono;
  (kphp::memory::libc_alloc_guard{},
   timelib_unixtime2local(now, chrono::time_point_cast<chrono::seconds>(chrono::system_clock::now()).time_since_epoch().count()));

  hou.transform([now](auto value) noexcept {
    now->h = value;
    return 0;
  });

  min.transform([now](auto value) noexcept {
    now->i = value;
    return 0;
  });

  sec.transform([now](auto value) noexcept {
    now->s = value;
    return 0;
  });
  mon.transform([now](auto value) noexcept {
    now->m = value;
    return 0;
  });

  day.transform([now](auto value) noexcept {
    now->d = value;
    return 0;
  });

  yea.transform([now](auto value) noexcept {
    now->y = value;
    return 0;
  });

  (kphp::memory::libc_alloc_guard{}, timelib_update_ts(now, tzinfo));

  return now->sse;
}

std::expected<void, error> modify(timelib_time* t, std::string_view modifier) noexcept {
  auto expected_tmp_time{construct_time(modifier)};

  if (!expected_tmp_time.has_value()) [[unlikely]] {
    return std::unexpected{std::move(expected_tmp_time).error()};
  }

  timelib_time* tmp_time{*expected_tmp_time};

  vk::final_action tmp_time_deleter{[tmp_time] { destruct(tmp_time); }};

  std::memcpy(&t->relative, &tmp_time->relative, sizeof(timelib_rel_time));
  t->have_relative = tmp_time->have_relative;
  t->sse_uptodate = 0;

  if (tmp_time->y != TIMELIB_UNSET) {
    t->y = tmp_time->y;
  }
  if (tmp_time->m != TIMELIB_UNSET) {
    t->m = tmp_time->m;
  }
  if (tmp_time->d != TIMELIB_UNSET) {
    t->d = tmp_time->d;
  }

  if (tmp_time->h != TIMELIB_UNSET) {
    t->h = tmp_time->h;
    if (tmp_time->i != TIMELIB_UNSET) {
      t->i = tmp_time->i;
      if (tmp_time->s != TIMELIB_UNSET) {
        t->s = tmp_time->s;
      } else {
        t->s = 0;
      }
    } else {
      t->i = 0;
      t->s = 0;
    }
  }

  if (tmp_time->us != TIMELIB_UNSET) {
    t->us = tmp_time->us;
  }

  timelib_update_ts(t, nullptr);
  timelib_update_from_sse(t);
  t->have_relative = 0;
  std::memset(&t->relative, 0, sizeof(t->relative));
  return {};
}

timelib_time& now(timelib_tzinfo* tzi) noexcept {
  timelib_time& res{(kphp::memory::libc_alloc_guard{}, *timelib_time_ctor())};

  res.tz_info = tzi;
  res.zone_type = TIMELIB_ZONETYPE_ID;

  namespace chrono = std::chrono;
  const auto time_since_epoch{chrono::system_clock::now().time_since_epoch()};
  const auto sec{chrono::duration_cast<chrono::seconds>(time_since_epoch).count()};
  const auto usec{chrono::duration_cast<chrono::microseconds>(time_since_epoch % chrono::seconds{1}).count()};

  timelib_unixtime2local(std::addressof(res), static_cast<timelib_sll>(sec));
  res.us = usec;

  return res;
}

std::optional<int64_t> strtotime(std::string_view timezone, std::string_view datetime, int64_t timestamp) noexcept {
  if (datetime.empty()) [[unlikely]] {
    kphp::log::warning("datetime can't be empty");
    return {};
  }

  int errc{}; // it's intentionally declared as 'int' since timelib_parse_tzfile accepts 'int'
  auto* tzinfo{kphp::timelib::get_timezone_info(timezone.data(), timelib_builtin_db(), std::addressof(errc))};
  if (tzinfo == nullptr) [[unlikely]] {
    kphp::log::warning("can't get timezone info: timezone -> {}, datetime -> {}, error -> {}", timezone, datetime, timelib_get_error_message(errc));
    return {};
  }

  timelib_time* now{timelib_time_ctor()};
  const vk::final_action now_deleter{[now] noexcept { timelib_time_dtor(now); }};
  now->tz_info = tzinfo;
  now->zone_type = TIMELIB_ZONETYPE_ID;
  timelib_unixtime2local(now, timestamp);

  auto expected_time{construct_time(datetime)};
  if (!expected_time.has_value()) [[unlikely]] {
    auto* errors{expected_time.error().err};
    kphp::log::warning("got {} errors in timelib_strtotime", errors->error_count); // TODO should we logs all the errors?
    return {};
  }
  time_t time{std::move(*expected_time)};

  errc = 0;
  timelib_fill_holes(time.get(), now, TIMELIB_NO_CLONE);
  timelib_update_ts(time.get(), tzinfo);
  int64_t result_timestamp{timelib_date_to_int(time.get(), std::addressof(errc))};
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
