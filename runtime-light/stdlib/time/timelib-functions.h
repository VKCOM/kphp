// Compiler for PHP (aka KPHP)
// Copyright (c) 2025 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <expected>
#include <format>
#include <optional>
#include <string_view>

#include "kphp/timelib/timelib.h"

#include "common/containers/final_action.h"
#include "runtime-common/core/std/containers.h"
#include "runtime-light/allocator/allocator.h"

namespace kphp::timelib {

constexpr inline std::array<std::string_view, 12> MON_FULL_NAMES = {"January", "February", "March",     "April",   "May",      "June",
                                                                    "July",    "August",   "September", "October", "November", "December"};
constexpr inline std::array<std::string_view, 12> MON_SHORT_NAMES = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
constexpr inline std::array<std::string_view, 7> DAY_FULL_NAMES = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};
constexpr inline std::array<std::string_view, 7> DAY_SHORT_NAMES = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};

struct error {
  timelib_error_container* err{nullptr};

  explicit error(timelib_error_container* err) noexcept
      : err{err} {}

  error(const error&) = delete;
  error& operator=(const error&) = delete;

  error(error&& other) noexcept {
    std::swap(err, other.err);
  }
  error& operator=(error&& other) noexcept {
    std::swap(err, other.err);
    return *this;
  }

  ~error();
};

struct destructor {
  void operator()(timelib_error_container* ec) const noexcept {
    kphp::memory::libc_alloc_guard{}, timelib_error_container_dtor(ec);
  }

  void operator()(timelib_rel_time* rt) const noexcept {
    kphp::memory::libc_alloc_guard{}, timelib_rel_time_dtor(rt);
  }

  void operator()(timelib_time* t) const noexcept {
    kphp::memory::libc_alloc_guard{}, timelib_time_dtor(t);
  }

  void operator()(timelib_time_offset* to) const noexcept {
    kphp::memory::libc_alloc_guard{}, timelib_time_offset_dtor(to);
  }
};

using error_container_t = std::unique_ptr<timelib_error_container, destructor>;
using rel_time_t = std::unique_ptr<timelib_rel_time, destructor>;
using time_t = std::unique_ptr<timelib_time, destructor>;
using time_offset_t = std::unique_ptr<timelib_time_offset, destructor>;

std::expected<time_t, error> construct_time(std::string_view time_sv) noexcept;
std::expected<time_t, error> construct_time(std::string_view time_sv, const char* format) noexcept;
time_offset_t construct_time_offset(timelib_time* t) noexcept;
std::expected<rel_time_t, error> construct_interval(std::string_view format) noexcept;

timelib_time* add(timelib_time* t, timelib_rel_time* interval) noexcept;

timelib_rel_time* clone(timelib_rel_time* rt) noexcept;
timelib_time* clone(timelib_time* t) noexcept;

timelib_rel_time* date_diff(timelib_time* time1, timelib_time* time2, bool absolute) noexcept;

std::string_view date_full_day_name(timelib_sll y, timelib_sll m, timelib_sll d) noexcept;

std::string_view date_short_day_name(timelib_sll y, timelib_sll m, timelib_sll d) noexcept;

int64_t date_timestamp_get(timelib_time& t) noexcept;

void date_timestamp_set(timelib_time* t, int64_t timestamp) noexcept;

const char* english_suffix(timelib_sll number) noexcept;

template<bool override_time>
void fill_holes(timelib_time& t, timelib_time* filler) noexcept {
  int options = TIMELIB_NO_CLONE;
  if constexpr (override_time) {
    options |= TIMELIB_OVERRIDE_TIME;
  }

  timelib_fill_holes(std::addressof(t), filler, options);
  timelib_update_ts(std::addressof(t), filler->tz_info);
  timelib_update_from_sse(std::addressof(t));

  t.have_relative = 0;
}

timelib_tzinfo* get_timezone_info(const char* timezone) noexcept;
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
timelib_tzinfo* get_timezone_info(const char* timezone, const timelib_tzdb* tzdb, int* errc) noexcept;

int64_t gmmktime(std::optional<int64_t> hou, std::optional<int64_t> min, std::optional<int64_t> sec, std::optional<int64_t> mon, std::optional<int64_t> day,
                 std::optional<int64_t> yea) noexcept;

bool is_leap_year(int32_t year) noexcept;

std::optional<int64_t> mktime(std::optional<int64_t> hou, std::optional<int64_t> min, std::optional<int64_t> sec, std::optional<int64_t> mon,
                              std::optional<int64_t> day, std::optional<int64_t> yea) noexcept;

std::expected<void, error> modify(timelib_time* t, std::string_view modifier) noexcept;

timelib_time& now(timelib_tzinfo* tzi) noexcept;

std::optional<int64_t> strtotime(std::string_view timezone, std::string_view datetime, int64_t timestamp) noexcept;

bool valid_date(int64_t year, int64_t month, int64_t day) noexcept;

template<typename OutputIt>
OutputIt date_format_to(OutputIt out, std::string_view format, timelib_time* t, bool localtime) noexcept {
  if (format.empty()) {
    return out;
  }

  time_offset_t offset{localtime ? construct_time_offset(t) : nullptr};

  bool weekYearSet{false};
  timelib_sll isoweek{};
  timelib_sll isoyear{};

  for (std::size_t i = 0; i < format.size(); ++i) {
    bool rfc_colon{false};
    switch (format[i]) {
    // day
    case 'd':
      out = std::format_to(out, "{:02}", t->d);
      break;
    case 'D':
      out = std::format_to(out, "{}", date_short_day_name(t->y, t->m, t->d));
      break;
    case 'j':
      out = std::format_to(out, "{}", t->d);
      break;
    case 'l':
      out = std::format_to(out, "{}", date_full_day_name(t->y, t->m, t->d));
      break;
    case 'S':
      out = std::format_to(out, "{}", english_suffix(t->d));
      break;
    case 'w':
      out = std::format_to(out, "{}", timelib_day_of_week(t->y, t->m, t->d));
      break;
    case 'N':
      out = std::format_to(out, "{}", timelib_iso_day_of_week(t->y, t->m, t->d));
      break;
    case 'z':
      out = std::format_to(out, "{}", timelib_day_of_year(t->y, t->m, t->d));
      break;

    // week
    case 'W':
      if (!weekYearSet) {
        timelib_isoweek_from_date(t->y, t->m, t->d, &isoweek, &isoyear);
        weekYearSet = true;
      }
      out = std::format_to(out, "{:02}", isoweek);
      break; // iso weeknr
    case 'o':
      if (!weekYearSet) {
        timelib_isoweek_from_date(t->y, t->m, t->d, &isoweek, &isoyear);
        weekYearSet = 1;
      }
      out = std::format_to(out, "{}", isoyear);
      break; // iso year

    // month
    case 'F':
      out = std::format_to(out, "{}", MON_FULL_NAMES[t->m - 1]);
      break;
    case 'm':
      out = std::format_to(out, "{:02}", t->m);
      break;
    case 'M':
      out = std::format_to(out, "{}", MON_SHORT_NAMES[t->m - 1]);
      break;
    case 'n':
      out = std::format_to(out, "{}", t->m);
      break;
    case 't':
      out = std::format_to(out, "{}", timelib_days_in_month(t->y, t->m));
      break;

    // year
    case 'L':
      out = std::format_to(out, "{}", is_leap_year(t->y));
      break;
    case 'y':
      out = std::format_to(out, "{:02}", t->y % 100);
      break;
    case 'Y':
      out = std::format_to(out, "{}{:04}", t->y < 0 ? "-" : "", std::abs(t->y));
      break;

    // time
    case 'a':
      out = std::format_to(out, "{}", t->h >= 12 ? "pm" : "am");
      break;
    case 'A':
      out = std::format_to(out, "{}", t->h >= 12 ? "PM" : "AM");
      break;
    case 'B': {
      auto retval{(((t->sse) - ((t->sse) - (((t->sse) % 86400) + 3600))) * 10)};
      if (retval < 0) {
        retval += 864000;
      }
      // Make sure to do this on a positive int to avoid rounding errors
      retval = (retval / 864) % 1000;
      out = std::format_to(out, "{:03}", retval);
      break;
    }
    case 'g':
      out = std::format_to(out, "{}", (t->h % 12) ? t->h % 12 : 12);
      break;
    case 'G':
      out = std::format_to(out, "{}", t->h);
      break;
    case 'h':
      out = std::format_to(out, "{:02}", (t->h % 12) ? t->h % 12 : 12);
      break;
    case 'H':
      out = std::format_to(out, "{:02}", t->h);
      break;
    case 'i':
      out = std::format_to(out, "{:02}", t->i);
      break;
    case 's':
      out = std::format_to(out, "{:02}", t->s);
      break;
    case 'u':
      out = std::format_to(out, "{:06}", t->us);
      break;
    case 'v':
      out = std::format_to(out, "{:03}", t->us / 1000);
      break;

    // timezone
    case 'I':
      out = std::format_to(out, "{}", localtime ? offset->is_dst : 0);
      break;
    case 'P':
      rfc_colon = true;
      [[fallthrough]];
    case 'O':
      out = std::format_to(out, "{}{:02}{}{:02}", localtime ? ((offset->offset < 0) ? '-' : '+') : '+', localtime ? std::abs(offset->offset / 3600) : 0,
                           rfc_colon ? ":" : "", localtime ? std::abs((offset->offset % 3600) / 60) : 0);
      break;
    case 'T':
      out = std::format_to(out, "{}", localtime ? offset->abbr : "GMT");
      break;
    case 'e':
      if (!localtime) {
        out = std::format_to(out, "UTC");
      } else {
        switch (t->zone_type) {
        case TIMELIB_ZONETYPE_ID:
          out = std::format_to(out, "{}", t->tz_info->name);
          break;
        case TIMELIB_ZONETYPE_ABBR:
          out = std::format_to(out, "{}", offset->abbr);
          break;
        case TIMELIB_ZONETYPE_OFFSET:
          out = std::format_to(out, "{}{:02}:{:02}", ((offset->offset < 0) ? '-' : '+'), abs(offset->offset / 3600), abs((offset->offset % 3600) / 60));
          break;
        }
      }
      break;
    case 'Z':
      out = std::format_to(out, "{}", localtime ? offset->offset : 0);
      break;

    // full date/time
    case 'c':
      out = std::format_to(out, "{:04}-{:02}-{:02}T{:02}:{:02}:{:02}{}{:02}:{:02}", t->y, t->m, t->d, t->h, t->i, t->s,
                           localtime ? ((offset->offset < 0) ? '-' : '+') : '+', localtime ? std::abs(offset->offset / 3600) : 0,
                           localtime ? std::abs((offset->offset % 3600) / 60) : 0);
      break;
    case 'r':
      out = std::format_to(out, "{:3}, {:02} {:3} {:04} {:02}:{:02}:{:02} {}{:02}{:02}", date_short_day_name(t->y, t->m, t->d), t->d, MON_SHORT_NAMES[t->m - 1],
                           t->y, t->h, t->i, t->s, localtime ? ((offset->offset < 0) ? '-' : '+') : '+', localtime ? abs(offset->offset / 3600) : 0,
                           localtime ? abs((offset->offset % 3600) / 60) : 0);
      break;
    case 'U':
      out = std::format_to(out, "{}", t->sse);
      break;

    case '\\':
      if (i < format.size()) {
        ++i;
      }
      [[fallthrough]];

    default:
      *out++ = format[i];
      break;
    }
  }

  return out;
}

template<typename OutputIt>
OutputIt date_format_to_localtime(OutputIt out, std::string_view format, timelib_time* t) noexcept {
  return date_format_to<OutputIt>(out, format, t, t->is_localtime);
}

template<typename OutputIt>
OutputIt date_interval_format_to(OutputIt out, std::string_view format, timelib_rel_time* t) noexcept {
  if (format.empty()) {
    return out;
  }

  bool have_format_spec{};

  for (auto c : format) {
    if (have_format_spec) {
      switch (c) {
      case 'Y':
        out = std::format_to(out, "{:02}", t->y);
        break;
      case 'y':
        out = std::format_to(out, "{}", t->y);
        break;

      case 'M':
        out = std::format_to(out, "{:02}", t->m);
        break;
      case 'm':
        out = std::format_to(out, "{}", t->m);
        break;

      case 'D':
        out = std::format_to(out, "{:02}", t->d);
        break;
      case 'd':
        out = std::format_to(out, "{}", t->d);
        break;

      case 'H':
        out = std::format_to(out, "{:02}", t->h);
        break;
      case 'h':
        out = std::format_to(out, "{}", t->h);
        break;

      case 'I':
        out = std::format_to(out, "{:02}", t->i);
        break;
      case 'i':
        out = std::format_to(out, "{}", t->i);
        break;

      case 'S':
        out = std::format_to(out, "{:02}", t->s);
        break;
      case 's':
        out = std::format_to(out, "{}", t->s);
        break;

      case 'F':
        out = std::format_to(out, "{:06}", t->us);
        break;
      case 'f':
        out = std::format_to(out, "{}", t->us);
        break;

      case 'a': {
        if (static_cast<int>(t->days) != TIMELIB_UNSET) {
          out = std::format_to(out, "{}", t->days);
        } else {
          out = std::format_to(out, "(unknown)");
        }
      } break;
      case 'r':
        out = std::format_to(out, "{}", t->invert ? "-" : "");
        break;
      case 'R':
        *out++ = (t->invert ? '-' : '+');
        break;

      case '%':
        *out++ = '%';
      default:
        *out++ = '%';
        *out++ = c;
        break;
      }
      have_format_spec = false;
    } else {
      if (c == '%') {
        have_format_spec = true;
      } else {
        *out++ = c;
      }
    }
  }
  return out;
}

template<bool override_time = false>
void fill_holes(timelib_time& time, timelib_tzinfo* tzi) noexcept {
  time_t now_time{std::addressof(now(tzi))};

  fill_holes<override_time>(time, now_time.get());
}

} // namespace kphp::timelib

template<>
struct std::formatter<kphp::timelib::error, char> {
  constexpr auto parse(format_parse_context& ctx) const {
    auto it = ctx.begin();

    if (it != ctx.end() && *it != '}') {
      throw format_error("Invalid format args for kphp::timelib::error.");
    }

    return it;
  }

  auto format(const kphp::timelib::error& error, auto& ctx) const noexcept {
    if (error.err != nullptr) {
      // spit out the first library error message, at least
      return format_to(ctx.out(), "at position {} ({}): {}", error.err->error_messages[0].position, error.err->error_messages[0].character,
                       error.err->error_messages[0].message);
    }
    return format_to(ctx.out(), "unknown error");
  }
};
