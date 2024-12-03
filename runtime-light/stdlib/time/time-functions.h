// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstdio>

#include "runtime-common/core/runtime-core.h"

inline int64_t f$_hrtime_int() noexcept {
  return std::chrono::steady_clock::now().time_since_epoch().count();
}

inline array<int64_t> f$_hrtime_array() noexcept {
  namespace chrono = std::chrono;
  const auto since_epoch = chrono::steady_clock::now().time_since_epoch();
  return array<int64_t>::create(duration_cast<chrono::seconds>(since_epoch).count(), chrono::nanoseconds{since_epoch % chrono::seconds{1}}.count());
}

inline mixed f$hrtime(bool as_number = false) noexcept {
  if (as_number) {
    return f$_hrtime_int();
  }
  return f$_hrtime_array();
}

inline string f$_microtime_string() noexcept {
  namespace chrono = std::chrono;
  const auto time_since_epoch{chrono::high_resolution_clock::now().time_since_epoch()};
  const auto seconds{duration_cast<chrono::seconds>(time_since_epoch).count()};
  const auto nanoseconds{duration_cast<chrono::nanoseconds>(time_since_epoch).count() % 1'000'000'000};

  constexpr size_t default_buffer_size = 60;
  char buf[default_buffer_size];
  const int len = snprintf(buf, default_buffer_size, "0.%09lld %lld", nanoseconds, seconds);
  return {buf, static_cast<string::size_type>(len)};
}

inline double f$_microtime_float() noexcept {
  namespace chrono = std::chrono;
  const auto time_since_epoch{chrono::system_clock::now().time_since_epoch()};
  const double microtime =
    duration_cast<chrono::seconds>(time_since_epoch).count() + (duration_cast<chrono::nanoseconds>(time_since_epoch).count() % 1'000'000'000) * 1e-9;
  return microtime;
}

inline mixed f$microtime(bool get_as_float = false) noexcept {
  if (get_as_float) {
    return f$_microtime_float();
  } else {
    return f$_microtime_string();
  }
}

inline int64_t f$time() noexcept {
  namespace chrono = std::chrono;
  const auto now{chrono::system_clock::now().time_since_epoch()};
  return duration_cast<chrono::seconds>(now).count();
}

int64_t f$mktime(Optional<int64_t> hour = {}, Optional<int64_t> minute = {}, Optional<int64_t> second = {}, Optional<int64_t> month = {},
                 Optional<int64_t> day = {}, Optional<int64_t> year = {}) noexcept;

string f$gmdate(const string &format, Optional<int64_t> timestamp = {}) noexcept;

string f$date(const string &format, Optional<int64_t> timestamp = {}) noexcept;

bool f$date_default_timezone_set(const string &s) noexcept;
