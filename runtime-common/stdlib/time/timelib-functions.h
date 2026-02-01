// Compiler for PHP (aka KPHP)
// Copyright (c) 2026 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <algorithm>
#include <cstdarg>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <iterator>

#include "kphp/timelib/timelib.h"

#include "runtime-common/core/runtime-core.h"
#include "runtime-common/core/utils/kphp-assert-core.h"
#include "runtime-common/stdlib/time/timelib-constants.h"

namespace kphp::timelib {

namespace details {

inline const char* english_suffix(timelib_sll number) noexcept {
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

inline const char* full_day_name(timelib_sll y, timelib_sll m, timelib_sll d) noexcept {
  timelib_sll day_of_week{timelib_day_of_week(y, m, d)};
  if (day_of_week < 0) {
    return "Unknown";
  }
  return kphp::timelib::days::FULL_NAMES[day_of_week];
}

inline const char* short_day_name(timelib_sll y, timelib_sll m, timelib_sll d) noexcept {
  timelib_sll day_of_week{timelib_day_of_week(y, m, d)};
  if (day_of_week < 0) {
    return "Unknown";
  }
  return kphp::timelib::days::SHORT_NAMES[day_of_week];
}

inline const char* minus_if_negative(timelib_sll number) noexcept {
  return number < 0 ? "-" : "";
}

inline char sign(int32_t number) noexcept {
  return number < 0 ? '-' : '+';
}

[[gnu::format(printf, 2, 3)]]
inline void format_to(string_buffer& sb, const char* format, ...) noexcept {
  // php implementation has less bytes buffer capacity
  static constexpr size_t MAX_SIZE{128};

  va_list args;
  va_start(args, format);
  sb.reserve(MAX_SIZE);
  auto actual_size{std::vsnprintf(std::next(sb.buffer(), sb.size()), MAX_SIZE, format, args)};
  va_end(args);
  php_assert(actual_size > 0);

  sb.set_pos(sb.size() + std::min(static_cast<size_t>(actual_size), MAX_SIZE));
}

} // namespace details

inline int64_t days_in_month(int64_t year, int64_t month) noexcept {
  return timelib_days_in_month(year, month);
}

inline bool valid_date(int64_t year, int64_t month, int64_t day) noexcept {
  return timelib_valid_date(year, month, day);
}

inline bool is_leap_year(int32_t year) noexcept {
  return (year % 4 == 0) && (year % 100 != 0 || year % 400 == 0);
}

string format_time(const string& format, timelib_time& t, timelib_time_offset* offset) noexcept;

inline string format_rel_time(const string& format, timelib_rel_time& t) noexcept {
  string_buffer& sb{RuntimeContext::get().static_SB_spare};
  sb.clean();

  bool have_format_spec{false};

  for (size_t i{0}; i < format.size(); ++i) {
    if (have_format_spec) {
      switch (format[i]) {
      case 'Y':
        kphp::timelib::details::format_to(sb, "%02lld", t.y);
        break;
      case 'y':
        kphp::timelib::details::format_to(sb, "%lld", t.y);
        break;

      case 'M':
        kphp::timelib::details::format_to(sb, "%02lld", t.m);
        break;
      case 'm':
        kphp::timelib::details::format_to(sb, "%lld", t.m);
        break;

      case 'D':
        kphp::timelib::details::format_to(sb, "%02lld", t.d);
        break;
      case 'd':
        kphp::timelib::details::format_to(sb, "%lld", t.d);
        break;

      case 'H':
        kphp::timelib::details::format_to(sb, "%02lld", t.h);
        break;
      case 'h':
        kphp::timelib::details::format_to(sb, "%lld", t.h);
        break;

      case 'I':
        kphp::timelib::details::format_to(sb, "%02lld", t.i);
        break;
      case 'i':
        kphp::timelib::details::format_to(sb, "%lld", t.i);
        break;

      case 'S':
        kphp::timelib::details::format_to(sb, "%02lld", t.s);
        break;
      case 's':
        kphp::timelib::details::format_to(sb, "%lld", t.s);
        break;

      case 'F':
        kphp::timelib::details::format_to(sb, "%06lld", t.us);
        break;
      case 'f':
        kphp::timelib::details::format_to(sb, "%lld", t.us);
        break;

      case 'a': {
        if (t.days != TIMELIB_UNSET) {
          kphp::timelib::details::format_to(sb, "%lld", t.days);
        } else {
          kphp::timelib::details::format_to(sb, "(unknown)");
        }
      } break;
      case 'r':
        kphp::timelib::details::format_to(sb, "%s", kphp::timelib::details::minus_if_negative(t.invert));
        break;
      case 'R':
        kphp::timelib::details::format_to(sb, "%c", t.invert ? '-' : '+');
        break;

      case '%':
        kphp::timelib::details::format_to(sb, "%%");
        break;
      default:
        sb << '%' << format[i];
        break;
      }
      have_format_spec = false;
    } else {
      if (format[i] == '%') {
        have_format_spec = true;
      } else {
        sb << format[i];
      }
    }
  }

  return sb.str();
}

string gen_error_msg(timelib_error_container* err) noexcept;

} // namespace kphp::timelib
