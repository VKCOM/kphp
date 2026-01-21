// Compiler for PHP (aka KPHP)
// Copyright (c) 2025 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <expected>
#include <format>
#include <iterator>
#include <optional>
#include <string_view>

#include "kphp/timelib/timelib.h"

#include "runtime-light/stdlib/time/timelib-types.h"

namespace kphp::timelib {

constexpr inline std::array<std::string_view, 12> MON_FULL_NAMES = {"January", "February", "March",     "April",   "May",      "June",
                                                                    "July",    "August",   "September", "October", "November", "December"};
constexpr inline std::array<std::string_view, 12> MON_SHORT_NAMES = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
constexpr inline std::array<std::string_view, 7> DAY_FULL_NAMES = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};
constexpr inline std::array<std::string_view, 7> DAY_SHORT_NAMES = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};

/* === rel_time === */
std::expected<kphp::timelib::rel_time, kphp::timelib::error_container> parse_interval(std::string_view s) noexcept;
kphp::timelib::rel_time clone_time_interval(const kphp::timelib::time& t) noexcept;
kphp::timelib::rel_time get_time_interval(const kphp::timelib::time& time1, const kphp::timelib::time& time2, bool absolute) noexcept;
kphp::timelib::time add_time_interval(const kphp::timelib::time& t, const kphp::timelib::rel_time& interval) noexcept;
std::expected<kphp::timelib::time, std::string_view> sub_time_interval(const kphp::timelib::time& t, const kphp::timelib::rel_time& interval) noexcept;

template<typename OutputIt>
OutputIt format_to(OutputIt out, std::string_view format, const kphp::timelib::rel_time& t) noexcept;

/* === time_offset === */
kphp::timelib::time_offset construct_time_offset(const kphp::timelib::time& t) noexcept;

/* === time === */
std::expected<std::pair<kphp::timelib::time, kphp::timelib::error_container>, kphp::timelib::error_container> parse_time(std::string_view s) noexcept;
std::expected<std::pair<kphp::timelib::time, kphp::timelib::error_container>, kphp::timelib::error_container> parse_time(std::string_view s,
                                                                                                                         std::string_view format) noexcept;
std::expected<std::pair<kphp::timelib::time, kphp::timelib::error_container>, kphp::timelib::error_container> parse_time(std::string_view s,
                                                                                                                         const kphp::timelib::time& t) noexcept;
kphp::timelib::time clone_time(const kphp::timelib::time& t) noexcept;

int64_t get_timestamp(const kphp::timelib::time& t) noexcept;
int64_t get_offset(const kphp::timelib::time& t) noexcept;
template<std::output_iterator<const char&> It>
It format_to(It out, std::string_view format, const kphp::timelib::time& t) noexcept;

void set_timestamp(kphp::timelib::time& t, int64_t timestamp) noexcept;
void set_date(kphp::timelib::time& t, int64_t y, int64_t m, int64_t d) noexcept;
void set_isodate(kphp::timelib::time& t, int64_t y, int64_t w, int64_t d) noexcept;
void set_time(kphp::timelib::time& t, int64_t h, int64_t i, int64_t s, int64_t ms) noexcept;

/* === timezone related ===*/
/**
 * @brief Retrieves a const reference to a `kphp::timelib::tzinfo` for a given time zone name.
 *
 * @param name The name of the time zone to retrieve.
 * @param tzdb The time zone database to search.
 * @return `const kphp::timelib::tzinfo&` holding the time zone information, or error code if not found.
 *
 * @note
 * - The returned object is owned by an internal cache.
 * - This function minimizes overhead by avoiding repeated allocations for the same time zone.
 */
std::expected<std::reference_wrapper<const kphp::timelib::tzinfo>, int32_t> get_timezone_info(std::string_view name,
                                                                                              const timelib_tzdb* tzdb = timelib_builtin_db()) noexcept;

/*=== timestamp ===*/
int64_t gmmktime(std::optional<int64_t> hou, std::optional<int64_t> min, std::optional<int64_t> sec, std::optional<int64_t> mon, std::optional<int64_t> day,
                 std::optional<int64_t> yea) noexcept;
std::optional<int64_t> mktime(std::optional<int64_t> hou, std::optional<int64_t> min, std::optional<int64_t> sec, std::optional<int64_t> mon,
                              std::optional<int64_t> day, std::optional<int64_t> yea) noexcept;
std::optional<int64_t> strtotime(std::string_view timezone, std::string_view s, int64_t now_timestamp) noexcept;

/* === helpers ===*/
bool valid_date(int64_t year, int64_t month, int64_t day) noexcept;
void fill_holes_with_now_info(kphp::timelib::time& time, const kphp::timelib::tzinfo& tzi, int32_t options = TIMELIB_NO_CLONE) noexcept;
void fill_holes_with_now_info(kphp::timelib::time& time, int32_t options = TIMELIB_NO_CLONE) noexcept;

namespace details {

std::string_view full_day_name(timelib_sll y, timelib_sll m, timelib_sll d) noexcept;
std::string_view short_day_name(timelib_sll y, timelib_sll m, timelib_sll d) noexcept;
std::string_view english_suffix(timelib_sll number) noexcept;

template<std::output_iterator<const char&> It>
It format_to(It out, std::string_view format, const kphp::timelib::time& t, bool localtime) noexcept {
  if (format.empty()) {
    return out;
  }

  kphp::timelib::time_offset offset{localtime ? kphp::timelib::construct_time_offset(t)
                                              : kphp::timelib::time_offset{nullptr, kphp::timelib::details::time_offset_destructor}};

  bool weekYearSet{false};
  timelib_sll isoweek{};
  timelib_sll isoyear{};

  for (size_t i{0}; i < format.size(); ++i) {
    bool rfc_colon{false};
    switch (format[i]) {
    // day
    case 'd':
      out = std::format_to(out, "{:02}", t->d);
      break;
    case 'D':
      out = std::format_to(out, "{}", kphp::timelib::details::short_day_name(t->y, t->m, t->d));
      break;
    case 'j':
      out = std::format_to(out, "{}", t->d);
      break;
    case 'l':
      out = std::format_to(out, "{}", kphp::timelib::details::full_day_name(t->y, t->m, t->d));
      break;
    case 'S':
      out = std::format_to(out, "{}", kphp::timelib::details::english_suffix(t->d));
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
        timelib_isoweek_from_date(t->y, t->m, t->d, std::addressof(isoweek), std::addressof(isoyear));
        weekYearSet = true;
      }
      out = std::format_to(out, "{:02}", isoweek);
      break; // iso weeknr
    case 'o':
      if (!weekYearSet) {
        timelib_isoweek_from_date(t->y, t->m, t->d, std::addressof(isoweek), std::addressof(isoyear));
        weekYearSet = true;
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
      out = std::format_to(out, "{:d}", std::chrono::year(t->y).is_leap());
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
      out = std::format_to(out, "{:3}, {:02} {:3} {:04} {:02}:{:02}:{:02} {}{:02}{:02}", short_day_name(t->y, t->m, t->d), t->d, MON_SHORT_NAMES[t->m - 1],
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

} // namespace details

template<std::output_iterator<const char&> It>
It format_to(It out, std::string_view format, const kphp::timelib::time& t) noexcept {
  return kphp::timelib::details::format_to<It>(out, format, t, t->is_localtime);
}

template<std::output_iterator<const char&> It>
It format_to(It out, std::string_view format, const kphp::timelib::rel_time& t) noexcept {
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
        if (t->days != TIMELIB_UNSET) {
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

} // namespace kphp::timelib

template<>
struct std::formatter<kphp::timelib::error_container, char> {
  constexpr auto parse(auto& ctx) const {
    return ctx.begin();
  }

  auto format(const kphp::timelib::error_container& error, auto& ctx) const noexcept {
    if (error != nullptr) {
      // spit out the first library error message, at least
      return format_to(ctx.out(), "at position {} ({}): {}", error->error_messages[0].position,
                       error->error_messages[0].character != '\0' ? error->error_messages[0].character : ' ', error->error_messages[0].message);
    }
    return format_to(ctx.out(), "unknown error");
  }
};
