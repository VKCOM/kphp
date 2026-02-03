// Compiler for PHP (aka KPHP)
// Copyright (c) 2025 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <expected>
#include <format>
#include <optional>
#include <string_view>

#include "kphp/timelib/timelib.h"

#include "runtime-light/stdlib/diagnostics/logs.h"
#include "runtime-light/stdlib/time/timelib-types.h"

namespace kphp::timelib {

std::expected<kphp::timelib::rel_time_holder, kphp::timelib::error_container_holder> parse_interval(std::string_view formatted_interval) noexcept;

kphp::timelib::rel_time_holder clone_time_interval(const kphp::timelib::time_holder& t) noexcept;

kphp::timelib::rel_time_holder get_time_interval(const kphp::timelib::time_holder& time1, const kphp::timelib::time_holder& time2, bool absolute) noexcept;

kphp::timelib::time_holder add_time_interval(const kphp::timelib::time_holder& t, const kphp::timelib::rel_time_holder& interval) noexcept;

std::expected<kphp::timelib::time_holder, std::string_view> sub_time_interval(const kphp::timelib::time_holder& t,
                                                                              const kphp::timelib::rel_time_holder& interval) noexcept;

// ================================================================================================

kphp::timelib::time_offset_holder construct_time_offset(const kphp::timelib::time_holder& t) noexcept;

// ================================================================================================

std::expected<std::pair<kphp::timelib::time_holder, kphp::timelib::error_container_holder>, kphp::timelib::error_container_holder>
parse_time(std::string_view formatted_time) noexcept;

std::expected<std::pair<kphp::timelib::time_holder, kphp::timelib::error_container_holder>, kphp::timelib::error_container_holder>
parse_time(std::string_view formatted_time, std::string_view format) noexcept;

std::expected<std::pair<kphp::timelib::time_holder, kphp::timelib::error_container_holder>, kphp::timelib::error_container_holder>
parse_time(std::string_view formatted_time, const kphp::timelib::time_holder& t) noexcept;

kphp::timelib::time_holder clone_time(const kphp::timelib::time_holder& t) noexcept;

int64_t get_timestamp(const kphp::timelib::time_holder& t) noexcept;

int64_t get_offset(const kphp::timelib::time_holder& t) noexcept;

void set_timestamp(kphp::timelib::time_holder& t, int64_t timestamp) noexcept;

void set_date(kphp::timelib::time_holder& t, int64_t y, int64_t m, int64_t d) noexcept;

void set_isodate(kphp::timelib::time_holder& t, int64_t y, int64_t w, int64_t d) noexcept;

void set_time(kphp::timelib::time_holder& t, int64_t h, int64_t i, int64_t s, int64_t ms) noexcept;

// ================================================================================================

/**
 * @brief Retrieves a const reference to a `kphp::timelib::tzinfo_holder` for a given time zone name.
 *
 * @param name The name of the time zone to retrieve.
 * @param tzdb The time zone database to search.
 * @return `const kphp::timelib::tzinfo_holder&` holding the time zone information, or error code if not found.
 *
 * @note
 * - The returned object is owned by an internal cache.
 * - This function minimizes overhead by avoiding repeated allocations for the same time zone.
 */
std::expected<std::reference_wrapper<const kphp::timelib::tzinfo_holder>, int32_t> get_timezone_info(std::string_view name,
                                                                                                     const timelib_tzdb* tzdb = timelib_builtin_db()) noexcept;

// ================================================================================================

int64_t gmmktime(std::optional<int64_t> hou, std::optional<int64_t> min, std::optional<int64_t> sec, std::optional<int64_t> mon, std::optional<int64_t> day,
                 std::optional<int64_t> yea) noexcept;

std::optional<int64_t> mktime(std::optional<int64_t> hou, std::optional<int64_t> min, std::optional<int64_t> sec, std::optional<int64_t> mon,
                              std::optional<int64_t> day, std::optional<int64_t> yea) noexcept;

std::optional<int64_t> strtotime(std::string_view timezone, std::string_view formatted_time, int64_t now_timestamp) noexcept;

// ================================================================================================

void fill_holes_with_now_info(kphp::timelib::time_holder& time, const kphp::timelib::tzinfo_holder& tzi, int32_t options = TIMELIB_NO_CLONE) noexcept;
void fill_holes_with_now_info(kphp::timelib::time_holder& time, int32_t options = TIMELIB_NO_CLONE) noexcept;

namespace details {

inline timelib_tzinfo* get_timezone_info(const char* timezone, const timelib_tzdb* tzdb, int* errc) noexcept {
  auto expected_tzinfo{kphp::timelib::get_timezone_info(timezone, tzdb)};
  if (!expected_tzinfo.has_value()) [[unlikely]] {
    *errc = expected_tzinfo.error();
    return nullptr;
  }
  return expected_tzinfo->get().get();
}

} // namespace details

inline kphp::timelib::time_offset_holder construct_time_offset(const kphp::timelib::time_holder& t) noexcept {
  if (t->zone_type == TIMELIB_ZONETYPE_ABBR) {
    kphp::timelib::time_offset_holder offset{timelib_time_offset_ctor(), kphp::timelib::details::time_offset_destructor};
    offset->offset = (t->z + (t->dst * 3600));
    offset->leap_secs = 0;
    offset->is_dst = t->dst;
    offset->abbr = timelib_strdup(t->tz_abbr);
    return offset;
  }
  if (t->zone_type == TIMELIB_ZONETYPE_OFFSET) {
    kphp::timelib::time_offset_holder offset{timelib_time_offset_ctor(), kphp::timelib::details::time_offset_destructor};
    offset->offset = (t->z);
    offset->leap_secs = 0;
    offset->is_dst = 0;
    offset->abbr = static_cast<char*>(timelib_malloc(9)); // GMT±xxxx\0
    // set upper bound to 99 just to ensure that 'hours_offset' fits in {:02}
    auto hours_offset{std::min(std::abs(offset->offset / 3600), 99)};
    *std::format_to_n(offset->abbr, 8, "GMT{}{:02}{:02}", (offset->offset < 0) ? '-' : '+', hours_offset, std::abs((offset->offset % 3600) / 60)).out = '\0';
    return offset;
  }
  return kphp::timelib::time_offset_holder{timelib_get_time_zone_info(t->sse, t->tz_info), kphp::timelib::details::time_offset_destructor};
}

inline std::expected<std::pair<kphp::timelib::time_holder, kphp::timelib::error_container_holder>, kphp::timelib::error_container_holder>
parse_time(std::string_view formatted_time) noexcept {
  timelib_error_container* raw_errors{};
  kphp::timelib::time_holder time{timelib_strtotime(formatted_time.data(), formatted_time.size(), std::addressof(raw_errors), timelib_builtin_db(),
                                                    kphp::timelib::details::get_timezone_info),
                                  kphp::timelib::details::time_destructor};
  kphp::timelib::error_container_holder errors{raw_errors};
  if (errors->error_count != 0) [[unlikely]] {
    return std::unexpected{std::move(errors)};
  }

  return std::make_pair(std::move(time), std::move(errors));
}

inline std::expected<std::pair<kphp::timelib::time_holder, kphp::timelib::error_container_holder>, kphp::timelib::error_container_holder>
parse_time(std::string_view formatted_time, std::string_view format) noexcept {
  timelib_error_container* raw_err{nullptr};
  kphp::timelib::time_holder t{timelib_parse_from_format(format.data(), formatted_time.data(), formatted_time.size(), std::addressof(raw_err),
                                                         timelib_builtin_db(), kphp::timelib::details::get_timezone_info),
                               kphp::timelib::details::time_destructor};
  kphp::timelib::error_container_holder err{raw_err};

  if (err && err->error_count) {
    return std::unexpected{std::move(err)};
  }

  return std::make_pair(std::move(t), std::move(err));
}

inline kphp::timelib::time_holder clone_time(const kphp::timelib::time_holder& t) noexcept {
  return kphp::timelib::time_holder{timelib_time_clone(t.get()), kphp::timelib::details::time_destructor};
}

inline int64_t get_timestamp(const kphp::timelib::time_holder& t) noexcept {
  timelib_update_ts(t.get(), nullptr);

  int error{}; // it's intentionally declared as 'int' since timelib_date_to_int accepts 'int'
  timelib_long timestamp{timelib_date_to_int(t.get(), std::addressof(error))};
  // the 'error' should be always 0 on x64 platform
  kphp::log::assertion(error == 0);

  return timestamp;
}

inline int64_t get_offset(const kphp::timelib::time_holder& t) noexcept {
  if (t->is_localtime) {
    switch (t->zone_type) {
    case TIMELIB_ZONETYPE_ID: {
      kphp::timelib::time_offset_holder offset{timelib_get_time_zone_info(t->sse, t->tz_info), kphp::timelib::details::time_offset_destructor};
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

inline void set_timestamp(kphp::timelib::time_holder& t, int64_t timestamp) noexcept {
  timelib_unixtime2local(t.get(), static_cast<timelib_sll>(timestamp));
  timelib_update_ts(t.get(), nullptr);
  t->us = 0;
}

inline void set_date(kphp::timelib::time_holder& t, int64_t y, int64_t m, int64_t d) noexcept {
  t->y = y;
  t->m = m;
  t->d = d;
  timelib_update_ts(t.get(), nullptr);
}

inline void set_isodate(kphp::timelib::time_holder& t, int64_t y, int64_t w, int64_t d) noexcept {
  t->y = y;
  t->m = 1;
  t->d = 1;
  std::memset(std::addressof(t->relative), 0, sizeof(t->relative));
  t->relative.d = timelib_daynr_from_weeknr(y, w, d);
  t->have_relative = 1;
  timelib_update_ts(t.get(), nullptr);
}

inline void set_time(kphp::timelib::time_holder& t, int64_t h, int64_t i, int64_t s, int64_t ms) noexcept {
  t->h = h;
  t->i = i;
  t->s = s;
  t->us = ms;
  timelib_update_ts(t.get(), nullptr);
}

inline kphp::timelib::rel_time_holder clone_time_interval(const kphp::timelib::time_holder& t) noexcept {
  return kphp::timelib::rel_time_holder{timelib_rel_time_clone(std::addressof(t->relative))};
}

inline kphp::timelib::rel_time_holder get_time_interval(const kphp::timelib::time_holder& time1, const kphp::timelib::time_holder& time2,
                                                        bool absolute) noexcept {
  timelib_update_ts(time1.get(), nullptr);
  timelib_update_ts(time2.get(), nullptr);

  kphp::timelib::rel_time_holder diff{timelib_diff(time1.get(), time2.get()), kphp::timelib::details::rel_time_destructor};
  if (absolute) {
    diff->invert = 0;
  }
  return diff;
}

inline kphp::timelib::time_holder add_time_interval(const kphp::timelib::time_holder& t, kphp::timelib::rel_time_holder& interval) noexcept {
  kphp::timelib::time_holder new_time{timelib_add(t.get(), interval.get()), kphp::timelib::details::time_destructor};
  return new_time;
}

inline std::expected<kphp::timelib::time_holder, std::string_view> sub_time_interval(const kphp::timelib::time_holder& t,
                                                                                     kphp::timelib::rel_time_holder& interval) noexcept {
  if (interval->have_special_relative) {
    return std::unexpected{"Only non-special relative time specifications are supported for subtraction"};
  }

  kphp::timelib::time_holder new_time{timelib_sub(t.get(), interval.get()), kphp::timelib::details::time_destructor};
  return new_time;
}

} // namespace kphp::timelib
