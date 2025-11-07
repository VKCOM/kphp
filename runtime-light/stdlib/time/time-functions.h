// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <limits>
#include <memory>

#include "runtime-common/core/runtime-core.h"
#include "runtime-light/k2-platform/k2-api.h"
#include "runtime-light/stdlib/diagnostics/logs.h"
#include "runtime-light/stdlib/time/time-state.h"
#include "runtime-light/stdlib/time/timelib-constants.h"
#include "runtime-light/stdlib/time/timelib-functions.h"

namespace kphp::time::impl {

constexpr inline std::array<std::string_view, 12> MON_FULL_NAMES = {"January", "February", "March",     "April",   "May",      "June",
                                                                    "July",    "August",   "September", "October", "November", "December"};
constexpr inline std::array<std::string_view, 12> MON_SHORT_NAMES = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
constexpr inline std::array<std::string_view, 7> DAY_FULL_NAMES = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};
constexpr inline std::array<std::string_view, 7> DAY_SHORT_NAMES = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};

constexpr inline int64_t CHECKDATE_YEAR_MIN = 1;
constexpr inline int64_t CHECKDATE_YEAR_MAX = 32767;

int64_t fix_year(int64_t year) noexcept;

string date(const string& format, const tm& t, int64_t timestamp, bool local) noexcept;

} // namespace kphp::time::impl

inline int64_t f$_hrtime_int() noexcept {
  return std::chrono::steady_clock::now().time_since_epoch().count();
}

inline array<int64_t> f$_hrtime_array() noexcept {
  namespace chrono = std::chrono;
  const auto since_epoch{chrono::steady_clock::now().time_since_epoch()};
  return array<int64_t>::create(duration_cast<chrono::seconds>(since_epoch).count(), chrono::nanoseconds{since_epoch % chrono::seconds{1}}.count());
}

inline mixed f$hrtime(bool as_number = false) noexcept {
  return as_number ? mixed{f$_hrtime_int()} : mixed{f$_hrtime_array()};
}

inline string f$_microtime_string() noexcept {
  namespace chrono = std::chrono;
  const auto time_since_epoch{chrono::high_resolution_clock::now().time_since_epoch()};
  const auto seconds{duration_cast<chrono::seconds>(time_since_epoch).count()};
  const auto nanoseconds{duration_cast<chrono::nanoseconds>(time_since_epoch).count() % 1'000'000'000};

  static constexpr size_t default_buffer_size = 60;
  char buf[default_buffer_size];
  const auto len{snprintf(buf, default_buffer_size, "0.%09lld %lld", nanoseconds, seconds)};
  return {buf, static_cast<string::size_type>(len)};
}

inline double f$_microtime_float() noexcept {
  namespace chrono = std::chrono;
  const auto time_since_epoch{chrono::system_clock::now().time_since_epoch()};
  const double microtime{duration_cast<chrono::seconds>(time_since_epoch).count() +
                         (duration_cast<chrono::nanoseconds>(time_since_epoch).count() % 1'000'000'000) * 1e-9};
  return microtime;
}

inline mixed f$microtime(bool as_float = false) noexcept {
  return as_float ? mixed{f$_microtime_float()} : mixed{f$_microtime_string()};
}

inline int64_t f$time() noexcept {
  namespace chrono = std::chrono;
  return duration_cast<chrono::seconds>(chrono::system_clock::now().time_since_epoch()).count();
}

inline int64_t f$mktime(int64_t hour = std::numeric_limits<int64_t>::min(), int64_t minute = std::numeric_limits<int64_t>::min(),
                        int64_t second = std::numeric_limits<int64_t>::min(), int64_t month = std::numeric_limits<int64_t>::min(),
                        int64_t day = std::numeric_limits<int64_t>::min(), int64_t year = std::numeric_limits<int64_t>::min()) noexcept {
  auto res{kphp::timelib::mktime(hour != std::numeric_limits<int64_t>::min() ? std::make_optional(hour) : std::nullopt,
                                 minute != std::numeric_limits<int64_t>::min() ? std::make_optional(minute) : std::nullopt,
                                 second != std::numeric_limits<int64_t>::min() ? std::make_optional(second) : std::nullopt,
                                 month != std::numeric_limits<int64_t>::min() ? std::make_optional(month) : std::nullopt,
                                 day != std::numeric_limits<int64_t>::min() ? std::make_optional(day) : std::nullopt,
                                 year != std::numeric_limits<int64_t>::min() ? std::make_optional(kphp::time::impl::fix_year(year)) : std::nullopt)};
  if (res.has_value()) {
    return *res;
  }
  return std::numeric_limits<int64_t>::min();
}

inline array<mixed> f$getdate(int64_t timestamp = std::numeric_limits<int64_t>::min()) noexcept {
  if (timestamp == std::numeric_limits<int64_t>::min()) {
    namespace chrono = std::chrono;
    timestamp = static_cast<int64_t>(chrono::time_point_cast<chrono::seconds>(chrono::system_clock::now()).time_since_epoch().count());
  }
  tm t{};
  time_t time{timestamp};
  tm* tp{k2::localtime_r(std::addressof(time), std::addressof(t))};
  if (tp == nullptr) {
    kphp::log::warning("unknown error in getdate with timestamp {}", timestamp);
    std::memset(std::addressof(t), 0, sizeof(tm));
  }

  array<mixed> result{array_size{11, false}};

  auto weekday{kphp::time::impl::DAY_FULL_NAMES[t.tm_wday]};
  auto month{kphp::time::impl::MON_FULL_NAMES[t.tm_mon]};

  result.set_value(string{"seconds", 7}, t.tm_sec);
  result.set_value(string{"minutes", 7}, t.tm_min);
  result.set_value(string{"hours", 5}, t.tm_hour);
  result.set_value(string{"mday", 4}, t.tm_mday);
  result.set_value(string{"wday", 4}, t.tm_wday);
  result.set_value(string{"mon", 3}, t.tm_mon + 1);
  result.set_value(string{"year", 4}, t.tm_year + 1900);
  result.set_value(string{"yday", 4}, t.tm_yday);
  result.set_value(string{"weekday", 7}, string{weekday.data(), static_cast<string::size_type>(weekday.size())});
  result.set_value(string{"month", 5}, string{month.data(), static_cast<string::size_type>(month.size())});
  result.set_value(string{"0", 1}, timestamp);

  return result;
}

inline string f$gmdate(const string& format, int64_t timestamp = std::numeric_limits<int64_t>::min()) noexcept {
  if (timestamp == std::numeric_limits<int64_t>::min()) {
    namespace chrono = std::chrono;
    timestamp = static_cast<int64_t>(chrono::time_point_cast<chrono::seconds>(chrono::system_clock::now()).time_since_epoch().count());
  }
  const time_t now{timestamp};
  struct tm tm {};
  gmtime_r(std::addressof(now), std::addressof(tm));
  return kphp::time::impl::date(format, tm, now, false);
}

inline int64_t f$gmmktime(int64_t hour = std::numeric_limits<int64_t>::min(), int64_t minute = std::numeric_limits<int64_t>::min(),
                          int64_t second = std::numeric_limits<int64_t>::min(), int64_t month = std::numeric_limits<int64_t>::min(),
                          int64_t day = std::numeric_limits<int64_t>::min(), int64_t year = std::numeric_limits<int64_t>::min()) noexcept {
  return kphp::timelib::gmmktime(hour != std::numeric_limits<int64_t>::min() ? std::make_optional(hour) : std::nullopt,
                                 minute != std::numeric_limits<int64_t>::min() ? std::make_optional(minute) : std::nullopt,
                                 second != std::numeric_limits<int64_t>::min() ? std::make_optional(second) : std::nullopt,
                                 month != std::numeric_limits<int64_t>::min() ? std::make_optional(month) : std::nullopt,
                                 day != std::numeric_limits<int64_t>::min() ? std::make_optional(day) : std::nullopt,
                                 year != std::numeric_limits<int64_t>::min() ? std::make_optional(kphp::time::impl::fix_year(year)) : std::nullopt);
}

inline string f$date(const string& format, int64_t timestamp = std::numeric_limits<int64_t>::min()) noexcept {
  if (timestamp == std::numeric_limits<int64_t>::min()) {
    namespace chrono = std::chrono;
    timestamp = static_cast<int64_t>(chrono::time_point_cast<chrono::seconds>(chrono::system_clock::now()).time_since_epoch().count());
  }
  const time_t now{timestamp};
  struct tm tm {};
  if (auto* res{k2::localtime_r(std::addressof(now), std::addressof(tm))}; res != std::addressof(tm)) [[unlikely]] {
    kphp::log::warning("can't get local time");
    return {};
  }
  return kphp::time::impl::date(format, tm, now, true);
}

inline string f$date_default_timezone_get() noexcept {
  return TimeInstanceState::get().default_timezone;
}

inline bool f$date_default_timezone_set(const string& timezone) noexcept {
  const std::string_view timezone_view{timezone.c_str(), timezone.size()};
  if (timezone_view != kphp::timelib::timezones::MOSCOW && timezone_view != kphp::timelib::timezones::GMT3) [[unlikely]] {
    kphp::log::warning("unsupported timezone '{}', only '{}' and '{}' are supported", timezone_view, kphp::timelib::timezones::MOSCOW,
                       kphp::timelib::timezones::GMT3);
    return false;
  }
  if (auto errc{k2::set_timezone(timezone_view)}; errc != k2::errno_ok) [[unlikely]] {
    kphp::log::warning("can't set timezone '{}'", timezone_view);
    return false;
  }
  TimeInstanceState::get().default_timezone = timezone;
  return true;
}

inline Optional<int64_t> f$strtotime(const string& datetime, int64_t base_timestamp = std::numeric_limits<int64_t>::min()) noexcept {
  if (base_timestamp == std::numeric_limits<int64_t>::min()) {
    namespace chrono = std::chrono;
    base_timestamp = static_cast<int64_t>(chrono::time_point_cast<chrono::seconds>(chrono::system_clock::now()).time_since_epoch().count());
  }
  string default_timezone{TimeInstanceState::get().default_timezone};
  const auto opt_timestamp{kphp::timelib::strtotime({default_timezone.c_str(), default_timezone.size()}, {datetime.c_str(), datetime.size()}, base_timestamp)};
  if (!opt_timestamp.has_value()) [[unlikely]] {
    return false;
  }
  return *opt_timestamp;
}

inline bool f$checkdate(int64_t month, int64_t day, int64_t year) noexcept {
  return year >= kphp::time::impl::CHECKDATE_YEAR_MIN && year <= kphp::time::impl::CHECKDATE_YEAR_MAX && kphp::timelib::valid_date(year, month, day);
}
