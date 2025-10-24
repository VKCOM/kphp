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

constexpr inline auto CHECKDATE_YEAR_MIN = 1;
constexpr inline auto CHECKDATE_YEAR_MAX = 32767;

int64_t fix_year(int64_t year) noexcept;

string date(const string& format, const tm& t, int64_t timestamp, bool local) noexcept;

string to_string(const std::string_view& sv) noexcept;

template<typename T>
std::optional<T> to_optional(const Optional<T>& kphp_opt) noexcept {
  return kphp_opt.has_value() ? std::make_optional(kphp_opt.val()) : std::nullopt;
}

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
  auto res{kphp::timelib::mktime(kphp::time::impl::to_optional(hour), kphp::time::impl::to_optional(minute), kphp::time::impl::to_optional(second),
                                 kphp::time::impl::to_optional(month), kphp::time::impl::to_optional(day), kphp::time::impl::to_optional(year))};
  if (res.has_value()) {
    return *res;
  }
  return std::numeric_limits<int64_t>::min();
}

inline array<mixed> f$getdate(Optional<int64_t> timestamp) noexcept {
  if (!timestamp.has_value()) {
    namespace chrono = std::chrono;
    timestamp = static_cast<int64_t>(chrono::time_point_cast<chrono::seconds>(chrono::system_clock::now()).time_since_epoch().count());
  }
  string default_timezone{TimeInstanceState::get().default_timezone};
  int errc{}; // it's intentionally declared as 'int' since timelib_parse_tzfile accepts 'int'
  auto* tzinfo{kphp::timelib::get_timezone_info(default_timezone.c_str(), timelib_builtin_db(), std::addressof(errc))};
  if (tzinfo == nullptr) [[unlikely]] {
    kphp::log::warning("can't get timezone info: timezone -> {}, error -> {}", default_timezone.c_str(), timelib_get_error_message(errc));
  }
  array<mixed> result(array_size(11, false));
  result.set_value(string{"0", 1}, timestamp.val());
  if (tzinfo == nullptr) [[unlikely]] {
    result.set_value(string{"seconds", 7}, 0);
    result.set_value(string{"minutes", 7}, 0);
    result.set_value(string{"hours", 5}, 0);
    result.set_value(string{"mday", 4}, 0);
    result.set_value(string{"wday", 4}, 0);
    result.set_value(string{"mon", 3}, 1);
    result.set_value(string{"year", 4}, 1900);
    result.set_value(string{"yday", 4}, 0);
    result.set_value(string{"weekday", 7}, kphp::time::impl::to_string(kphp::timelib::DAY_FULL_NAMES[0]));
    result.set_value(string{"month", 5}, kphp::time::impl::to_string(kphp::timelib::MON_FULL_NAMES[0]));

    return result;
  }

  auto [seconds, minutes, hours, mday, wday, mon, year, yday, weekday, month] = kphp::timelib::getdate(timestamp.val(), *tzinfo);

  result.set_value(string{"seconds", 7}, seconds);
  result.set_value(string{"minutes", 7}, minutes);
  result.set_value(string{"hours", 5}, hours);
  result.set_value(string{"mday", 4}, mday);
  result.set_value(string{"wday", 4}, wday);
  result.set_value(string{"mon", 3}, mon);
  result.set_value(string{"year", 4}, year);
  result.set_value(string{"yday", 4}, yday);
  result.set_value(string{"weekday", 7}, kphp::time::impl::to_string(weekday));
  result.set_value(string{"month", 5}, kphp::time::impl::to_string(month));

  return result;
}

inline string f$gmdate(const string& format, Optional<int64_t> timestamp = {}) noexcept {
  namespace chrono = std::chrono;
  const time_t now{timestamp.has_value() ? timestamp.val() : duration_cast<chrono::seconds>(chrono::system_clock::now().time_since_epoch()).count()};
  struct tm tm {};
  gmtime_r(std::addressof(now), std::addressof(tm));
  return kphp::time::impl::date(format, tm, now, false);
}

inline int64_t f$gmmktime(Optional<int64_t> hour = {}, Optional<int64_t> minute = {}, Optional<int64_t> second = {}, Optional<int64_t> month = {},
                          Optional<int64_t> day = {}, Optional<int64_t> year = {}) noexcept {
  return kphp::timelib::gmmktime(kphp::time::impl::to_optional(hour), kphp::time::impl::to_optional(minute), kphp::time::impl::to_optional(second),
                                 kphp::time::impl::to_optional(month), kphp::time::impl::to_optional(day), kphp::time::impl::to_optional(year));
}

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
