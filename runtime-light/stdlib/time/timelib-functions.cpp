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

std::pair<time_t, error_container_t> construct_time(std::string_view time_sv) noexcept {
  timelib_error_container* errors{};
  time_t time{(kphp::memory::libc_alloc_guard{},
               timelib_strtotime(time_sv.data(), time_sv.size(), std::addressof(errors), timelib_builtin_db(), kphp::timelib::get_timezone_info))};
  if (errors->error_count != 0) [[unlikely]] {
    return {nullptr, error_container_t{errors}};
  }

  return {std::move(time), error_container_t{errors}};
}

std::pair<time_t, error_container_t> construct_time(std::string_view time_sv, const char* format) noexcept {
  timelib_error_container* err{nullptr};
  time_t t{(kphp::memory::libc_alloc_guard{},
            timelib_parse_from_format(format, time_sv.data(), time_sv.size(), std::addressof(err), timelib_builtin_db(), kphp::timelib::get_timezone_info))};

  if (err && err->error_count) {
    return {nullptr, error_container_t{err}};
  }

  return {std::move(t), error_container_t{err}};
}

time_offset_t construct_time_offset(timelib_time& t) noexcept {
  if (t.zone_type == TIMELIB_ZONETYPE_ABBR) {
    time_offset_t offset{(kphp::memory::libc_alloc_guard{}, timelib_time_offset_ctor())};
    offset->offset = (t.z + (t.dst * 3600));
    offset->leap_secs = 0;
    offset->is_dst = t.dst;
    offset->abbr = (kphp::memory::libc_alloc_guard{}, timelib_strdup(t.tz_abbr));
    return offset;
  }
  if (t.zone_type == TIMELIB_ZONETYPE_OFFSET) {
    time_offset_t offset{(kphp::memory::libc_alloc_guard{}, timelib_time_offset_ctor())};
    offset->offset = (t.z);
    offset->leap_secs = 0;
    offset->is_dst = 0;
    offset->abbr = (kphp::memory::libc_alloc_guard{}, static_cast<char*>(timelib_malloc(9))); // GMT±xxxx\0
    // set upper bound to 99 just to ensure that 'hours_offset' fits in {:02}
    auto hours_offset{std::min(std::abs(offset->offset / 3600), 99)};
    *std::format_to_n(offset->abbr, 8, "GMT{}{:02}{:02}", (offset->offset < 0) ? '-' : '+', hours_offset, std::abs((offset->offset % 3600) / 60)).out = '\0';
    return offset;
  }
  return time_offset_t{(kphp::memory::libc_alloc_guard{}, timelib_get_time_zone_info(t.sse, t.tz_info))};
}

std::expected<rel_time_t, std::format_string<const char*>> construct_interval(std::string_view format) noexcept {
  timelib_time* b{nullptr};
  timelib_time* e{nullptr};
  vk::final_action e_deleter{[e]() { kphp::memory::libc_alloc_guard{}, free(e); }};
  vk::final_action b_deleter{[b]() { kphp::memory::libc_alloc_guard{}, free(b); }};
  timelib_rel_time* p{nullptr};
  int r{}; // it's intentionally declared as 'int' since timelib_strtointerval accepts 'int'
  timelib_error_container* errors{nullptr};

  kphp::memory::libc_alloc_guard{},
      timelib_strtointerval(format.data(), format.size(), std::addressof(b), std::addressof(e), std::addressof(p), std::addressof(r), std::addressof(errors));

  if (errors->error_count > 0) {
    rel_time_t{p};
    error_container_t{errors};
    return std::unexpected{std::format_string<const char*>{"Unknown or bad format ({})"}};
  }
  error_container_t{errors};

  if (p != nullptr) {
    return rel_time_t{p};
  }

  if (b != nullptr && e != nullptr) {
    timelib_update_ts(b, nullptr);
    timelib_update_ts(e, nullptr);
    return kphp::memory::libc_alloc_guard{}, rel_time_t{timelib_diff(b, e)};
  }

  return std::unexpected{std::format_string<const char*>{"Failed to parse interval ({})"}};
}

time_t add(timelib_time& t, timelib_rel_time& interval) noexcept {
  time_t new_time{(kphp::memory::libc_alloc_guard{}, timelib_add(std::addressof(t), std::addressof(interval)))};
  return new_time;
}

rel_time_t clone(timelib_rel_time& rt) noexcept {
  return rel_time_t{(kphp::memory::libc_alloc_guard{}, timelib_rel_time_clone(std::addressof(rt)))};
}

time_t clone(timelib_time& t) noexcept {
  return time_t{(kphp::memory::libc_alloc_guard{}, timelib_time_clone(std::addressof(t)))};
}

rel_time_t diff(timelib_time& time1, timelib_time& time2, bool absolute) noexcept {
  kphp::memory::libc_alloc_guard{}, timelib_update_ts(std::addressof(time1), nullptr);
  kphp::memory::libc_alloc_guard{}, timelib_update_ts(std::addressof(time2), nullptr);

  rel_time_t diff{(kphp::memory::libc_alloc_guard{}, timelib_diff(std::addressof(time1), std::addressof(time2)))};
  if (absolute) {
    diff->invert = 0;
  }
  return diff;
}

std::string_view date_full_day_name(timelib_sll y, timelib_sll m, timelib_sll d) noexcept {
  timelib_sll day_of_week{timelib_day_of_week(y, m, d)};
  if (day_of_week < 0) {
    return "Unknown";
  }
  return DAY_FULL_NAMES[day_of_week];
}

std::string_view date_short_day_name(timelib_sll y, timelib_sll m, timelib_sll d) noexcept {
  timelib_sll day_of_week{timelib_day_of_week(y, m, d)};
  if (day_of_week < 0) {
    return "Unknown";
  }
  return DAY_SHORT_NAMES[day_of_week];
}

int64_t get_timestamp(timelib_time& t) noexcept {
  kphp::memory::libc_alloc_guard{}, timelib_update_ts(std::addressof(t), nullptr);

  int error{}; // it's intentionally declared as 'int' since timelib_date_to_int accepts 'int'
  timelib_long timestamp{timelib_date_to_int(std::addressof(t), std::addressof(error))};
  // the 'error' should be always 0 on x64 platform
  log::assertion(error == 0);

  return timestamp;
}

void set_timestamp(timelib_time& t, int64_t timestamp) noexcept {
  kphp::memory::libc_alloc_guard{}, timelib_unixtime2local(std::addressof(t), static_cast<timelib_sll>(timestamp));
  kphp::memory::libc_alloc_guard{}, timelib_update_ts(std::addressof(t), nullptr);
  t.us = 0;
}

std::string_view english_suffix(timelib_sll number) noexcept {
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

error_container_t modify(timelib_time& t, std::string_view modifier) noexcept {
  auto [tmp_time, errors]{construct_time(modifier)};

  if (tmp_time == nullptr) [[unlikely]] {
    return std::move(errors);
  }

  std::memcpy(std::addressof(t.relative), std::addressof(tmp_time->relative), sizeof(timelib_rel_time));
  t.have_relative = tmp_time->have_relative;
  t.sse_uptodate = 0;

  if (tmp_time->y != TIMELIB_UNSET) {
    t.y = tmp_time->y;
  }
  if (tmp_time->m != TIMELIB_UNSET) {
    t.m = tmp_time->m;
  }
  if (tmp_time->d != TIMELIB_UNSET) {
    t.d = tmp_time->d;
  }

  if (tmp_time->h != TIMELIB_UNSET) {
    t.h = tmp_time->h;
    if (tmp_time->i != TIMELIB_UNSET) {
      t.i = tmp_time->i;
      if (tmp_time->s != TIMELIB_UNSET) {
        t.s = tmp_time->s;
      } else {
        t.s = 0;
      }
    } else {
      t.i = 0;
      t.s = 0;
    }
  }

  if (tmp_time->us != TIMELIB_UNSET) {
    t.us = tmp_time->us;
  }

  kphp::memory::libc_alloc_guard{}, timelib_update_ts(std::addressof(t), nullptr);
  kphp::memory::libc_alloc_guard{}, timelib_update_from_sse(std::addressof(t));
  t.have_relative = 0;
  std::memset(std::addressof(t.relative), 0, sizeof(t.relative));
  return std::move(errors);
}

time_t now(timelib_tzinfo* tzi) noexcept {
  time_t res{(kphp::memory::libc_alloc_guard{}, timelib_time_ctor())};

  res->tz_info = tzi;
  res->zone_type = TIMELIB_ZONETYPE_ID;

  namespace chrono = std::chrono;
  const auto time_since_epoch{chrono::system_clock::now().time_since_epoch()};
  const auto sec{chrono::duration_cast<chrono::seconds>(time_since_epoch).count()};
  const auto usec{chrono::duration_cast<chrono::microseconds>(time_since_epoch % chrono::seconds{1}).count()};

  kphp::memory::libc_alloc_guard{}, timelib_unixtime2local(res.get(), static_cast<timelib_sll>(sec));
  res->us = usec;

  return res;
}

void set_date(timelib_time& t, int64_t y, int64_t m, int64_t d) noexcept {
  t.y = y;
  t.m = m;
  t.d = d;
  kphp::memory::libc_alloc_guard{}, timelib_update_ts(std::addressof(t), nullptr);
}

void set_isodate(timelib_time& t, int64_t y, int64_t w, int64_t d) noexcept {
  t.y = y;
  t.m = 1;
  t.d = 1;
  std::memset(std::addressof(t.relative), 0, sizeof(t.relative));
  t.relative.d = timelib_daynr_from_weeknr(y, w, d);
  t.have_relative = 1;
  kphp::memory::libc_alloc_guard{}, timelib_update_ts(std::addressof(t), nullptr);
}

void set_time(timelib_time& t, int64_t h, int64_t i, int64_t s, int64_t ms) noexcept {
  t.h = h;
  t.i = i;
  t.s = s;
  t.us = ms;
  kphp::memory::libc_alloc_guard{}, timelib_update_ts(std::addressof(t), nullptr);
}

std::expected<time_t, std::string_view> sub(timelib_time& t, timelib_rel_time& interval) noexcept {
  if (interval.have_special_relative) {
    return std::unexpected{"Only non-special relative time specifications are supported for subtraction"};
  }

  time_t new_time{(kphp::memory::libc_alloc_guard{}, timelib_sub(std::addressof(t), std::addressof(interval)))};
  return new_time;
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

  kphp::memory::libc_alloc_guard _{};

  timelib_time* now{timelib_time_ctor()};
  const vk::final_action now_deleter{[now] noexcept { timelib_time_dtor(now); }};
  now->tz_info = tzinfo;
  now->zone_type = TIMELIB_ZONETYPE_ID;
  timelib_unixtime2local(now, timestamp);

  auto [time, errors]{construct_time(datetime)};
  if (time == nullptr) [[unlikely]] {
    kphp::log::warning("got {} errors in timelib_strtotime", errors->error_count); // TODO should we logs all the errors?
    return {};
  }

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
