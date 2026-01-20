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
#include "runtime-light/stdlib/time/timelib-types.h"

namespace kphp::timelib {

namespace {

/**
 * @brief Retrieves a pointer to a `timelib_tzinfo` structure for a given time zone name.
 *
 * @param timezone The name of the time zone to retrieve.
 * @param tzdb The time zone database to search.
 * @param errc The pointer to a variable to store error code into.
 * @return `timelib_tzinfo*` pointing to the time zone information, or `nullptr` if not found.
 *
 * @note
 * - The returned pointer is owned by an internal cache; do not deallocate it using `timelib_tzinfo_dtor`.
 * - This function minimizes overhead by avoiding repeated allocations for the same time zone.
 */
timelib_tzinfo* get_cached_timezone_info(const char* timezone, const timelib_tzdb* tzdb, int* errc) noexcept {
  auto expected_tzinfo{kphp::timelib::get_cached_timezone_info(timezone, tzdb)};
  if (!expected_tzinfo.has_value()) [[unlikely]] {
    *errc = expected_tzinfo.error();
    return nullptr;
  }
  return expected_tzinfo->get().get();
}

} // namespace

time_offset construct_time_offset(const kphp::timelib::time& t) noexcept {
  if (t->zone_type == TIMELIB_ZONETYPE_ABBR) {
    time_offset offset{(kphp::memory::libc_alloc_guard{}, timelib_time_offset_ctor())};
    offset->offset = (t->z + (t->dst * 3600));
    offset->leap_secs = 0;
    offset->is_dst = t->dst;
    offset->abbr = (kphp::memory::libc_alloc_guard{}, timelib_strdup(t->tz_abbr));
    return offset;
  }
  if (t->zone_type == TIMELIB_ZONETYPE_OFFSET) {
    time_offset offset{(kphp::memory::libc_alloc_guard{}, timelib_time_offset_ctor())};
    offset->offset = (t->z);
    offset->leap_secs = 0;
    offset->is_dst = 0;
    offset->abbr = (kphp::memory::libc_alloc_guard{}, static_cast<char*>(timelib_malloc(9))); // GMT±xxxx\0
    // set upper bound to 99 just to ensure that 'hours_offset' fits in {:02}
    auto hours_offset{std::min(std::abs(offset->offset / 3600), 99)};
    *std::format_to_n(offset->abbr, 8, "GMT{}{:02}{:02}", (offset->offset < 0) ? '-' : '+', hours_offset, std::abs((offset->offset % 3600) / 60)).out = '\0';
    return offset;
  }
  return time_offset{(kphp::memory::libc_alloc_guard{}, timelib_get_time_zone_info(t->sse, t->tz_info))};
}

std::expected<std::pair<kphp::timelib::time, kphp::timelib::error_container>, kphp::timelib::error_container> parse_time(std::string_view time_sv) noexcept {
  timelib_error_container* errors{};
  time time{(kphp::memory::libc_alloc_guard{},
             timelib_strtotime(time_sv.data(), time_sv.size(), std::addressof(errors), timelib_builtin_db(), get_cached_timezone_info))};
  if (errors->error_count != 0) [[unlikely]] {
    return std::unexpected{error_container{errors}};
  }

  return std::make_pair(std::move(time), error_container{errors});
}

std::expected<std::pair<kphp::timelib::time, kphp::timelib::error_container>, kphp::timelib::error_container> parse_time(std::string_view time_sv,
                                                                                                                         std::string_view format_sv) noexcept {
  timelib_error_container* err{nullptr};
  time t{(kphp::memory::libc_alloc_guard{},
          timelib_parse_from_format(format_sv.data(), time_sv.data(), time_sv.size(), std::addressof(err), timelib_builtin_db(), get_cached_timezone_info))};

  if (err && err->error_count) {
    return std::unexpected{error_container{err}};
  }

  return std::make_pair(std::move(t), error_container{err});
}

std::expected<rel_time, error_container> parse_interval(std::string_view interval_sv) noexcept {
  timelib_time* b{nullptr};
  timelib_time* e{nullptr};
  vk::final_action e_deleter{[e]() { kphp::memory::libc_alloc_guard{}, free(e); }};
  vk::final_action b_deleter{[b]() { kphp::memory::libc_alloc_guard{}, free(b); }};
  timelib_rel_time* p{nullptr};
  int r{}; // it's intentionally declared as 'int' since timelib_strtointerval accepts 'int'
  timelib_error_container* errors{nullptr};

  kphp::memory::libc_alloc_guard{}, timelib_strtointerval(interval_sv.data(), interval_sv.size(), std::addressof(b), std::addressof(e), std::addressof(p),
                                                          std::addressof(r), std::addressof(errors));

  if (errors->error_count > 0) {
    kphp::timelib::details::rel_time_destructor{}(p);
    return std::unexpected{error_container{errors}};
  }

  if (p != nullptr) {
    return rel_time{p};
  }

  if (b != nullptr && e != nullptr) {
    timelib_update_ts(b, nullptr);
    timelib_update_ts(e, nullptr);
    return kphp::memory::libc_alloc_guard{}, rel_time{timelib_diff(b, e)};
  }

  return std::unexpected{error_container{errors}};
}

time add_time_interval(const kphp::timelib::time& t, const kphp::timelib::rel_time& interval) noexcept {
  time new_time{(kphp::memory::libc_alloc_guard{}, timelib_add(t.get(), interval.get()))};
  return new_time;
}

rel_time clone_time_interval(const time& t) noexcept {
  return rel_time{(kphp::memory::libc_alloc_guard{}, timelib_rel_time_clone(std::addressof(t->relative)))};
}

time clone_time(const kphp::timelib::time& t) noexcept {
  return time{(kphp::memory::libc_alloc_guard{}, timelib_time_clone(t.get()))};
}

rel_time get_time_interval(const kphp::timelib::time& time1, const kphp::timelib::time& time2, bool absolute) noexcept {
  kphp::memory::libc_alloc_guard{}, timelib_update_ts(time1.get(), nullptr);
  kphp::memory::libc_alloc_guard{}, timelib_update_ts(time2.get(), nullptr);

  rel_time diff{(kphp::memory::libc_alloc_guard{}, timelib_diff(time1.get(), time2.get()))};
  if (absolute) {
    diff->invert = 0;
  }
  return diff;
}

int64_t get_timestamp(const kphp::timelib::time& t) noexcept {
  kphp::memory::libc_alloc_guard{}, timelib_update_ts(t.get(), nullptr);

  int error{}; // it's intentionally declared as 'int' since timelib_date_to_int accepts 'int'
  timelib_long timestamp{timelib_date_to_int(t.get(), std::addressof(error))};
  // the 'error' should be always 0 on x64 platform
  log::assertion(error == 0);

  return timestamp;
}

void set_timestamp(const kphp::timelib::time& t, int64_t timestamp) noexcept {
  kphp::memory::libc_alloc_guard{}, timelib_unixtime2local(t.get(), static_cast<timelib_sll>(timestamp));
  kphp::memory::libc_alloc_guard{}, timelib_update_ts(t.get(), nullptr);
  t->us = 0;
}

int64_t get_offset(const kphp::timelib::time& t) noexcept {
  if (t->is_localtime) {
    switch (t->zone_type) {
    case TIMELIB_ZONETYPE_ID: {
      time_offset offset{(kphp::memory::libc_alloc_guard{}, timelib_get_time_zone_info(t->sse, t->tz_info))};
      int64_t offset_int{offset->offset};
      return offset_int;
    }
    case TIMELIB_ZONETYPE_OFFSET: {
      return t->z;
    }
    case TIMELIB_ZONETYPE_ABBR: {
      return t->z + (3600 * t->dst);
    }
    }
  }
  return 0;
}

std::expected<std::reference_wrapper<const kphp::timelib::tzinfo>, int32_t> get_cached_timezone_info(std::string_view timezone_sv,
                                                                                                     const timelib_tzdb* tzdb) noexcept {
  auto opt_tzinfo{TimeImageState::get().timelib_zone_cache.get(timezone_sv)};
  if (opt_tzinfo.has_value()) {
    return *opt_tzinfo;
  }
  auto& instance_timelib_zone_cache{TimeInstanceState::get().timelib_zone_cache};
  opt_tzinfo = instance_timelib_zone_cache.get(timezone_sv);
  if (opt_tzinfo.has_value()) {
    return *opt_tzinfo;
  }

  return instance_timelib_zone_cache.make(timezone_sv, tzdb);
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
  auto expected_tzinfo{kphp::timelib::get_cached_timezone_info({default_timezone.c_str(), default_timezone.size()})};
  if (!expected_tzinfo.has_value()) [[unlikely]] {
    kphp::log::warning("can't get timezone info: timezone -> {}, error -> {}", default_timezone.c_str(), timelib_get_error_message(expected_tzinfo.error()));
    return std::nullopt;
  }
  const auto& tzinfo{expected_tzinfo->get()};
  now->tz_info = tzinfo.get();
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

  (kphp::memory::libc_alloc_guard{}, timelib_update_ts(now, tzinfo.get()));

  return now->sse;
}

std::expected<std::pair<kphp::timelib::time, kphp::timelib::error_container>, kphp::timelib::error_container> parse_time(std::string_view time_sv,
                                                                                                                         const time& t) noexcept {
  auto expected{parse_time(time_sv)};

  if (!expected.has_value()) [[unlikely]] {
    return expected;
  }

  auto& [tmp_time, errors]{*expected};
  auto res{kphp::timelib::clone_time(t)};

  std::memcpy(std::addressof(res->relative), std::addressof(tmp_time->relative), sizeof(timelib_rel_time));
  res->have_relative = tmp_time->have_relative;
  res->sse_uptodate = 0;

  if (tmp_time->y != TIMELIB_UNSET) {
    res->y = tmp_time->y;
  }
  if (tmp_time->m != TIMELIB_UNSET) {
    res->m = tmp_time->m;
  }
  if (tmp_time->d != TIMELIB_UNSET) {
    res->d = tmp_time->d;
  }

  if (tmp_time->h != TIMELIB_UNSET) {
    res->h = tmp_time->h;
    if (tmp_time->i != TIMELIB_UNSET) {
      res->i = tmp_time->i;
      if (tmp_time->s != TIMELIB_UNSET) {
        res->s = tmp_time->s;
      } else {
        res->s = 0;
      }
    } else {
      res->i = 0;
      res->s = 0;
    }
  }

  if (tmp_time->us != TIMELIB_UNSET) {
    res->us = tmp_time->us;
  }

  kphp::memory::libc_alloc_guard{}, timelib_update_ts(res.get(), nullptr);
  kphp::memory::libc_alloc_guard{}, timelib_update_from_sse(res.get());
  res->have_relative = 0;
  std::memset(std::addressof(res->relative), 0, sizeof(res->relative));
  return std::make_pair(std::move(res), std::move(errors));
}

void set_date(const kphp::timelib::time& t, int64_t y, int64_t m, int64_t d) noexcept {
  t->y = y;
  t->m = m;
  t->d = d;
  kphp::memory::libc_alloc_guard{}, timelib_update_ts(t.get(), nullptr);
}

void set_isodate(const kphp::timelib::time& t, int64_t y, int64_t w, int64_t d) noexcept {
  t->y = y;
  t->m = 1;
  t->d = 1;
  std::memset(std::addressof(t->relative), 0, sizeof(t->relative));
  t->relative.d = timelib_daynr_from_weeknr(y, w, d);
  t->have_relative = 1;
  kphp::memory::libc_alloc_guard{}, timelib_update_ts(t.get(), nullptr);
}

void set_time(const kphp::timelib::time& t, int64_t h, int64_t i, int64_t s, int64_t ms) noexcept {
  t->h = h;
  t->i = i;
  t->s = s;
  t->us = ms;
  kphp::memory::libc_alloc_guard{}, timelib_update_ts(t.get(), nullptr);
}

std::expected<time, std::string_view> sub_time_interval(const kphp::timelib::time& t, const kphp::timelib::rel_time& interval) noexcept {
  if (interval->have_special_relative) {
    return std::unexpected{"Only non-special relative time specifications are supported for subtraction"};
  }

  time new_time{(kphp::memory::libc_alloc_guard{}, timelib_sub(t.get(), interval.get()))};
  return new_time;
}

std::optional<int64_t> strtotime(std::string_view timezone, std::string_view datetime, int64_t timestamp) noexcept {
  if (datetime.empty()) [[unlikely]] {
    kphp::log::warning("datetime can't be empty");
    return {};
  }

  auto expected_tzinfo{kphp::timelib::get_cached_timezone_info(timezone)};
  if (!expected_tzinfo.has_value()) [[unlikely]] {
    kphp::log::warning("can't get timezone info: timezone -> {}, datetime -> {}, error -> {}", timezone, datetime,
                       timelib_get_error_message(expected_tzinfo.error()));
    return {};
  }
  const auto& tzinfo{expected_tzinfo->get()};

  kphp::memory::libc_alloc_guard _{};

  timelib_time* now{timelib_time_ctor()};
  const vk::final_action now_deleter{[now] noexcept { timelib_time_dtor(now); }};
  now->tz_info = tzinfo.get();
  now->zone_type = TIMELIB_ZONETYPE_ID;
  timelib_unixtime2local(now, timestamp);

  auto expected{parse_time(datetime)};
  if (!expected.has_value()) [[unlikely]] {
    kphp::log::warning("got {} errors in timelib_strtotime", expected.error()->error_count); // TODO should we logs all the errors?
    return {};
  }

  int errc{}; // it's intentionally declared as 'int' since timelib_date_to_int accepts 'int'
  auto& [time, errors]{*expected};
  timelib_fill_holes(time.get(), now, TIMELIB_NO_CLONE);
  timelib_update_ts(time.get(), tzinfo.get());
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

namespace details {

std::string_view full_day_name(timelib_sll y, timelib_sll m, timelib_sll d) noexcept {
  timelib_sll day_of_week{timelib_day_of_week(y, m, d)};
  if (day_of_week < 0) {
    return "Unknown";
  }
  return DAY_FULL_NAMES[day_of_week];
}

std::string_view short_day_name(timelib_sll y, timelib_sll m, timelib_sll d) noexcept {
  timelib_sll day_of_week{timelib_day_of_week(y, m, d)};
  if (day_of_week < 0) {
    return "Unknown";
  }
  return DAY_SHORT_NAMES[day_of_week];
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

} // namespace details

} // namespace kphp::timelib
