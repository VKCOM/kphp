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
#include "runtime-light/allocator/allocator.h"
#include "runtime-light/stdlib/diagnostics/logs.h"
#include "runtime-light/stdlib/time/time-state.h"

namespace kphp::timelib {

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

} // namespace kphp::timelib
