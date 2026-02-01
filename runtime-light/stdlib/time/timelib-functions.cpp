// Compiler for PHP (aka KPHP)
// Copyright (c) 2025 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime-light/stdlib/time/timelib-functions.h"

#include <chrono>
#include <cstdint>
#include <cstring>
#include <expected>
#include <memory>
#include <optional>
#include <string_view>
#include <utility>

#include "kphp/timelib/timelib.h"

#include "common/containers/final_action.h"
#include "runtime-common/core/allocator/script-malloc-interface.h"
#include "runtime-light/stdlib/diagnostics/logs.h"
#include "runtime-light/stdlib/time/time-state.h"
#include "runtime-light/stdlib/time/timelib-types.h"

/*
 * This is necessary to replace the library's allocators.
 * When building the library, the prefix of the replacement function is specified, for example:
 * if we want to use the prefix of the replacement function = "my_prefix_", then during assembly we specify
 *    cmake -DTIMELIB_ALLOC_FUNC_PREFIX=my_prefix_ .
 * then it will use
 *   void* my_prefix_malloc(size_t size);
 *   void* my_prefix_realloc(void* ptr, size_t size);
 *   void* my_prefix_calloc(size_t num, size_t size);
 *   char* my_prefix_strdup(const char* str);
 *   void my_prefix_free(void* ptr);
 * instead of
 *   void* malloc(size_t size);
 *   void* realloc(void* ptr, size_t size);
 *   void* calloc(size_t num, size_t size);
 *   char* strdup(const char* str);
 *   void free(void* ptr);
 */
extern "C" {
void* timelib_malloc(size_t size) {
  return kphp::memory::script::alloc(size);
}
void* timelib_realloc(void* ptr, size_t size) {
  return kphp::memory::script::realloc(ptr, size);
}
void* timelib_calloc(size_t num, size_t size) {
  return kphp::memory::script::calloc(num, size);
}
char* timelib_strdup(const char* str1) {
  return kphp::memory::script::strdup(str1);
}
void timelib_free(void* ptr) {
  kphp::memory::script::free(ptr);
}
}

namespace kphp::timelib {

std::expected<kphp::timelib::rel_time_holder, kphp::timelib::error_container_holder> parse_interval(std::string_view formatted_interval) noexcept {
  timelib_time* raw_b{nullptr};
  timelib_time* raw_e{nullptr};
  timelib_rel_time* raw_p{nullptr};
  int r{}; // it's intentionally declared as 'int' since timelib_strtointerval accepts 'int'
  timelib_error_container* raw_errors{nullptr};

  timelib_strtointerval(formatted_interval.data(), formatted_interval.size(), std::addressof(raw_b), std::addressof(raw_e), std::addressof(raw_p),
                        std::addressof(r), std::addressof(raw_errors));

  kphp::timelib::time_holder e{raw_e};
  kphp::timelib::time_holder b{raw_b};
  kphp::timelib::rel_time_holder p{raw_p};
  kphp::timelib::error_container_holder errors{raw_errors};

  if (errors->error_count > 0) {
    return std::unexpected{std::move(errors)};
  }

  if (p != nullptr) {
    return p;
  }

  if (b != nullptr && e != nullptr) {
    timelib_update_ts(b.get(), nullptr);
    timelib_update_ts(e.get(), nullptr);
    return kphp::timelib::rel_time_holder{timelib_diff(b.get(), e.get()), kphp::timelib::details::rel_time_destructor};
  }

  return std::unexpected{std::move(errors)};
}

std::expected<std::reference_wrapper<const kphp::timelib::tzinfo_holder>, int32_t> get_timezone_info(std::string_view name, const timelib_tzdb* tzdb) noexcept {
  auto opt_tzinfo{TimeImageState::get().timelib_zone_cache.get(name)};
  if (opt_tzinfo.has_value()) {
    return *opt_tzinfo;
  }
  auto& instance_timelib_zone_cache{TimeInstanceState::get().timelib_zone_cache};
  opt_tzinfo = instance_timelib_zone_cache.get(name);
  if (opt_tzinfo.has_value()) {
    return *opt_tzinfo;
  }

  int errc{}; // it's intentionally declared as 'int' since timelib_parse_tzfile accepts 'int'
  kphp::timelib::tzinfo_holder tzinfo{timelib_parse_tzfile(name.data(), tzdb, std::addressof(errc)), kphp::timelib::details::tzinfo_destructor};
  if (tzinfo == nullptr || tzinfo->name == nullptr) [[unlikely]] {
    return std::unexpected{errc};
  }
  return *instance_timelib_zone_cache.put(std::move(tzinfo));
}

int64_t gmmktime(std::optional<int64_t> hou, std::optional<int64_t> min, std::optional<int64_t> sec, std::optional<int64_t> mon, std::optional<int64_t> day,
                 std::optional<int64_t> yea) noexcept {
  auto* now{timelib_time_ctor()};
  const vk::final_action now_deleter{[now] noexcept { timelib_time_dtor(now); }};
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
  auto* now{timelib_time_ctor()};
  const vk::final_action now_deleter{[now] noexcept { timelib_time_dtor(now); }};
  string default_timezone{TimeInstanceState::get().default_timezone};
  auto expected_tzinfo{kphp::timelib::get_timezone_info({default_timezone.c_str(), default_timezone.size()})};
  if (!expected_tzinfo.has_value()) [[unlikely]] {
    kphp::log::warning("can't get timezone info: timezone -> {}, error -> {}", default_timezone.c_str(), timelib_get_error_message(expected_tzinfo.error()));
    return std::nullopt;
  }
  const auto& tzinfo{expected_tzinfo->get()};
  now->tz_info = tzinfo.get();
  now->zone_type = TIMELIB_ZONETYPE_ID;
  namespace chrono = std::chrono;
  timelib_unixtime2local(now, chrono::time_point_cast<chrono::seconds>(chrono::system_clock::now()).time_since_epoch().count());

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

  timelib_update_ts(now, tzinfo.get());

  return now->sse;
}

std::expected<std::pair<kphp::timelib::time_holder, kphp::timelib::error_container_holder>, kphp::timelib::error_container_holder>
parse_time(std::string_view formatted_time, const kphp::timelib::time_holder& t) noexcept {
  auto expected{kphp::timelib::parse_time(formatted_time)};

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

  timelib_update_ts(res.get(), nullptr);
  timelib_update_from_sse(res.get());
  res->have_relative = 0;
  std::memset(std::addressof(res->relative), 0, sizeof(res->relative));
  return std::make_pair(std::move(res), std::move(errors));
}

std::optional<int64_t> strtotime(std::string_view timezone, std::string_view formatted_time, int64_t timestamp) noexcept {
  if (formatted_time.empty()) [[unlikely]] {
    kphp::log::warning("datetime can't be empty");
    return {};
  }

  auto expected_tzinfo{kphp::timelib::get_timezone_info(timezone)};
  if (!expected_tzinfo.has_value()) [[unlikely]] {
    kphp::log::warning("can't get timezone info: timezone -> {}, datetime -> {}, error -> {}", timezone, formatted_time,
                       timelib_get_error_message(expected_tzinfo.error()));
    return {};
  }
  const auto& tzinfo{expected_tzinfo->get()};

  kphp::timelib::time_holder now{timelib_time_ctor()};
  now->tz_info = tzinfo.get();
  now->zone_type = TIMELIB_ZONETYPE_ID;
  timelib_unixtime2local(now.get(), timestamp);

  auto expected{kphp::timelib::parse_time(formatted_time)};
  if (!expected.has_value()) [[unlikely]] {
    kphp::log::warning("got {} errors in timelib_strtotime", expected.error()->error_count); // TODO should we logs all the errors?
    return {};
  }

  int errc{}; // it's intentionally declared as 'int' since timelib_date_to_int accepts 'int'
  auto& [time, errors]{*expected};
  timelib_fill_holes(time.get(), now.get(), TIMELIB_NO_CLONE);
  timelib_update_ts(time.get(), tzinfo.get());
  int64_t result_timestamp{timelib_date_to_int(time.get(), std::addressof(errc))};
  if (errc != 0) [[unlikely]] {
    kphp::log::warning("can't convert date to int: error -> {}", timelib_get_error_message(errc));
    return {};
  }
  return result_timestamp;
}

void fill_holes_with_now_info(kphp::timelib::time_holder& time, const kphp::timelib::tzinfo_holder& tzi, int32_t options) noexcept {
  namespace chrono = std::chrono;
  const auto time_since_epoch{chrono::system_clock::now().time_since_epoch()};
  const auto sec{chrono::duration_cast<chrono::seconds>(time_since_epoch).count()};
  const auto usec{chrono::duration_cast<chrono::microseconds>(time_since_epoch % chrono::seconds{1}).count()};

  kphp::timelib::time_holder now{timelib_time_ctor(), kphp::timelib::details::time_destructor};

  now->tz_info = tzi.get();
  now->zone_type = TIMELIB_ZONETYPE_ID;

  timelib_unixtime2local(now.get(), static_cast<timelib_sll>(sec));
  now->us = usec;

  timelib_fill_holes(time.get(), now.get(), options);
  timelib_update_ts(time.get(), now->tz_info);
  timelib_update_from_sse(time.get());

  time->have_relative = 0;
}

void fill_holes_with_now_info(kphp::timelib::time_holder& time, int32_t options) noexcept {
  namespace chrono = std::chrono;
  const auto time_since_epoch{chrono::system_clock::now().time_since_epoch()};
  const auto sec{chrono::duration_cast<chrono::seconds>(time_since_epoch).count()};
  const auto usec{chrono::duration_cast<chrono::microseconds>(time_since_epoch % chrono::seconds{1}).count()};

  kphp::timelib::time_holder now{timelib_time_ctor(), kphp::timelib::details::time_destructor};

  now->tz_info = time->tz_info;
  now->zone_type = TIMELIB_ZONETYPE_ID;

  timelib_unixtime2local(now.get(), static_cast<timelib_sll>(sec));
  now->us = usec;

  timelib_fill_holes(time.get(), now.get(), options);
  timelib_update_ts(time.get(), now->tz_info);
  timelib_update_from_sse(time.get());

  time->have_relative = 0;
}

} // namespace kphp::timelib
