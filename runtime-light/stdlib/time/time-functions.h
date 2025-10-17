// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <limits>

#include "runtime-common/core/runtime-core.h"
#include "runtime-light/k2-platform/k2-api.h"
#include "runtime-light/stdlib/diagnostics/logs.h"
#include "runtime-light/stdlib/time/time-state.h"
#include "runtime-light/stdlib/time/timelib-constants.h"
#include "runtime-light/stdlib/time/timelib-functions.h"

namespace kphp::time::impl {

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

inline int64_t f$mktime(Optional<int64_t> hour = {}, Optional<int64_t> minute = {}, Optional<int64_t> second = {}, Optional<int64_t> month = {},
                        Optional<int64_t> day = {}, Optional<int64_t> year = {}) noexcept {
  namespace chrono = std::chrono;
  const auto time_since_epoch{chrono::system_clock::now().time_since_epoch()};
  chrono::year_month_day current_date{chrono::sys_days{duration_cast<chrono::days>(time_since_epoch)}};

  const auto hours{chrono::hours(hour.has_value() ? hour.val() : duration_cast<chrono::hours>(time_since_epoch).count() % 24)};
  const auto minutes{chrono::minutes(minute.has_value() ? minute.val() : duration_cast<chrono::minutes>(time_since_epoch).count() % 60)};
  const auto seconds{chrono::seconds(second.has_value() ? second.val() : duration_cast<chrono::seconds>(time_since_epoch).count() % 60)};
  const auto months{chrono::months(month.has_value() ? month.val() : static_cast<unsigned>(current_date.month()))};
  const auto days{chrono::days(day.has_value() ? day.val() : static_cast<unsigned>(current_date.day()))};
  const auto years{chrono::years(year.has_value() ? kphp::time::impl::fix_year(year.val()) : static_cast<int>(current_date.year()) - 1970)};

  const auto result{hours + minutes + seconds + months + days + years};
  return duration_cast<chrono::seconds>(result).count();
}

array<mixed> f$getdate(int64_t timestamp = std::numeric_limits<int64_t>::min()) noexcept;

inline string f$gmdate(const string& format, Optional<int64_t> timestamp = {}) noexcept {
  namespace chrono = std::chrono;
  const time_t now{timestamp.has_value() ? timestamp.val() : duration_cast<chrono::seconds>(chrono::system_clock::now().time_since_epoch()).count()};
  struct tm tm {};
  gmtime_r(std::addressof(now), std::addressof(tm));
  return kphp::time::impl::date(format, tm, now, false);
}

int64_t f$gmmktime(int64_t h = std::numeric_limits<int64_t>::min(), int64_t m = std::numeric_limits<int64_t>::min(),
                   int64_t s = std::numeric_limits<int64_t>::min(), int64_t month = std::numeric_limits<int64_t>::min(),
                   int64_t day = std::numeric_limits<int64_t>::min(), int64_t year = std::numeric_limits<int64_t>::min()) noexcept;

inline string f$date(const string& format, Optional<int64_t> timestamp = {}) noexcept {
  namespace chrono = std::chrono;
  const time_t now{timestamp.has_value() ? timestamp.val() : duration_cast<chrono::seconds>(chrono::system_clock::now().time_since_epoch()).count()};
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
  string default_timezone{f$date_default_timezone_get()};
  const auto opt_timestamp{kphp::timelib::strtotime({default_timezone.c_str(), default_timezone.size()}, {datetime.c_str(), datetime.size()}, base_timestamp)};
  if (!opt_timestamp.has_value()) [[unlikely]] {
    return false;
  }
  return *opt_timestamp;
}

bool f$checkdate(int64_t month, int64_t day, int64_t year) noexcept;
