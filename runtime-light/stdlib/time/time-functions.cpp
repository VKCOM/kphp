// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime-light/stdlib/time/time-functions.h"

#include <array>
#include <chrono>
#include <climits>
#include <cstdint>
#include <ctime>
#include <memory>
#include <string_view>

#include "runtime-common/core/runtime-core.h"
#include "runtime-light/stdlib/time/timelib-constants.h"

namespace {

constexpr std::array<std::string_view, 12> PHP_TIMELIB_MON_FULL_NAMES = {"January", "February", "March",     "April",   "May",      "June",
                                                                         "July",    "August",   "September", "October", "November", "December"};
constexpr std::array<std::string_view, 12> PHP_TIMELIB_MON_SHORT_NAMES = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
constexpr std::array<std::string_view, 7> PHP_TIMELIB_DAY_FULL_NAMES = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};
constexpr std::array<std::string_view, 7> PHP_TIMELIB_DAY_SHORT_NAMES = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};

constexpr std::array<std::string_view, 4> suffix = {"st", "nd", "rd", "th"};

void iso_week_number(int y, int doy, int weekday, int& iw, int& iy) noexcept {
  int y_leap = std::chrono::year(y).is_leap();
  int prev_y_leap = std::chrono::year(y - 1).is_leap();
  int jan1weekday = (weekday - (doy % 7) + 7) % 7;

  if (weekday == 0) {
    weekday = 7;
  }
  if (jan1weekday == 0) {
    jan1weekday = 7;
  }
  /* Find if Y M D falls in YearNumber Y-1, WeekNumber 52 or 53 */
  if (doy <= (7 - jan1weekday) && jan1weekday > 4) {
    iy = y - 1;
    if (jan1weekday == 5 || (jan1weekday == 6 && prev_y_leap)) {
      iw = 53;
    } else {
      iw = 52;
    }
  } else {
    iy = y;
  }
  /* Find if Y M D falls in YearNumber Y+1, WeekNumber 1 */
  if (iy == y) {
    int i = y_leap ? 366 : 365;
    if ((i - (doy - y_leap + 1)) < (4 - weekday)) {
      iy = y + 1;
      iw = 1;
      return;
    }
  }
  /* Find if Y M D falls in YearNumber Y, WeekNumber 1 through 53 */
  if (iy == y) {
    int j = doy + (7 - weekday) + jan1weekday;
    iw = j / 7;
    if (jan1weekday > 4) {
      iw -= 1;
    }
  }
}

string date(const string& format, const tm& t, int64_t timestamp, bool local) noexcept {
  string_buffer& SB{RuntimeContext::get().static_SB};

  int year = t.tm_year + 1900;
  int month = t.tm_mon + 1;
  int day = t.tm_mday;
  int hour = t.tm_hour;
  int hour12 = (hour + 11) % 12 + 1;
  int minute = t.tm_min;
  int second = t.tm_sec;
  int day_of_week = t.tm_wday;
  int day_of_year = t.tm_yday;
  int64_t internet_time = 0;
  int iso_week = 0;
  int iso_year = 0;

  SB.clean();
  for (int i = 0; i < static_cast<int>(format.size()); i++) {
    switch (format[i]) {
    case 'd':
      SB << static_cast<char>(day / 10 + '0') << static_cast<char>(day % 10 + '0');
      break;
    case 'D':
      SB << PHP_TIMELIB_DAY_SHORT_NAMES[day_of_week].data();
      break;
    case 'j':
      SB << day;
      break;
    case 'l':
      SB << PHP_TIMELIB_DAY_FULL_NAMES[day_of_week].data();
      break;
    case 'N':
      SB << (day_of_week == 0 ? '7' : static_cast<char>(day_of_week + '0'));
      break;
    case 'S': {
      int c = INT_MAX;
      switch (day) {
      case 1:
      case 21:
      case 31:
        c = 0;
        break;
      case 2:
      case 22:
        c = 1;
        break;
      case 3:
      case 23:
        c = 2;
        break;
      default:
        c = 3;
      }
      SB << suffix[c].data();
      break;
    }
    case 'w':
      SB << static_cast<char>(day_of_week + '0');
      break;
    case 'z':
      SB << day_of_year;
      break;
    case 'W':
      iso_week_number(year, day_of_year, day_of_week, iso_week, iso_year);
      SB << static_cast<char>('0' + iso_week / 10) << static_cast<char>('0' + iso_week % 10);
      break;
    case 'F':
      SB << PHP_TIMELIB_MON_FULL_NAMES[month - 1].data();
      break;
    case 'm':
      SB << static_cast<char>(month / 10 + '0') << static_cast<char>(month % 10 + '0');
      break;
    case 'M':
      SB << PHP_TIMELIB_MON_SHORT_NAMES[month - 1].data();
      break;
    case 'n':
      SB << month;
      break;
    case 't':
      SB << static_cast<unsigned>(
          std::chrono::year_month_day_last(std::chrono::year(year), std::chrono::month_day_last{std::chrono::month(month) / std::chrono::last}).day());
      break;
    case 'L':
      SB << static_cast<int>(std::chrono::year(year).is_leap());
      break;
    case 'o':
      iso_week_number(year, day_of_year, day_of_week, iso_week, iso_year);
      SB << iso_year;
      break;
    case 'Y':
      SB << year;
      break;
    case 'y':
      SB << static_cast<char>(year / 10 % 10 + '0') << static_cast<char>(year % 10 + '0');
      break;
    case 'a':
      SB << (hour < 12 ? "am" : "pm");
      break;
    case 'A':
      SB << (hour < 12 ? "AM" : "PM");
      break;
    case 'B':
      internet_time = (timestamp + 3600) % 86400 * 1000 / 86400;
      SB << static_cast<char>(internet_time / 100 + '0') << static_cast<char>((internet_time / 10) % 10 + '0') << static_cast<char>(internet_time % 10 + '0');
      break;
    case 'g':
      SB << hour12;
      break;
    case 'G':
      SB << hour;
      break;
    case 'h':
      SB << static_cast<char>(hour12 / 10 + '0') << static_cast<char>(hour12 % 10 + '0');
      break;
    case 'H':
      SB << static_cast<char>(hour / 10 + '0');
      SB << static_cast<char>(hour % 10 + '0');
      break;
    case 'i':
      SB << static_cast<char>(minute / 10 + '0');
      SB << static_cast<char>(minute % 10 + '0');
      break;
    case 's':
      SB << static_cast<char>(second / 10 + '0');
      SB << static_cast<char>(second % 10 + '0');
      break;
    case 'u':
      SB << "000000";
      break;
    case 'e':
      if (local) {
        SB << kphp::timelib::timezones::MOSCOW.data();
      } else {
        SB << "UTC";
      }
      break;
    case 'I':
      SB << static_cast<int>(t.tm_isdst > 0);
      break;
    case 'O':
      if (local) {
        SB << "+0300";
      } else {
        SB << "+0000";
      }
      break;
    case 'P':
      if (local) {
        SB << "+03:00";
      } else {
        SB << "+00:00";
      }
      break;
    case 'T':
      if (local) {
        SB << "MSK";
      } else {
        SB << "GMT";
      }
      break;
    case 'Z':
      if (local) {
        SB << 3 * 3600;
      } else {
        SB << 0;
      }
      break;
    case 'c':
      SB << year;
      SB << '-';
      SB << static_cast<char>(month / 10 + '0');
      SB << static_cast<char>(month % 10 + '0');
      SB << '-';
      SB << static_cast<char>(day / 10 + '0');
      SB << static_cast<char>(day % 10 + '0');
      SB << "T";
      SB << static_cast<char>(hour / 10 + '0');
      SB << static_cast<char>(hour % 10 + '0');
      SB << ':';
      SB << static_cast<char>(minute / 10 + '0');
      SB << static_cast<char>(minute % 10 + '0');
      SB << ':';
      SB << static_cast<char>(second / 10 + '0');
      SB << static_cast<char>(second % 10 + '0');
      if (local) {
        SB << "+03:00";
      } else {
        SB << "+00:00";
      }
      break;
    case 'r':
      SB << PHP_TIMELIB_DAY_SHORT_NAMES[day_of_week].data();
      SB << ", ";
      SB << static_cast<char>(day / 10 + '0');
      SB << static_cast<char>(day % 10 + '0');
      SB << ' ';
      SB << PHP_TIMELIB_MON_SHORT_NAMES[month - 1].data();
      SB << ' ';
      SB << year;
      SB << ' ';
      SB << static_cast<char>(hour / 10 + '0');
      SB << static_cast<char>(hour % 10 + '0');
      SB << ':';
      SB << static_cast<char>(minute / 10 + '0');
      SB << static_cast<char>(minute % 10 + '0');
      SB << ':';
      SB << static_cast<char>(second / 10 + '0');
      SB << static_cast<char>(second % 10 + '0');
      if (local) {
        SB << " +0300";
      } else {
        SB << " +0000";
      }
      break;
    case 'U':
      SB << timestamp;
      break;
    case '\\':
      if (format[i + 1]) {
        i++;
      }
      /* fallthrough */
    default:
      SB << format[i];
    }
  }
  return SB.str();
}

int64_t fix_year(int64_t year) noexcept {
  if (year <= 100U) {
    if (year <= 69) {
      year += 2000;
    } else {
      year += 1900;
    }
  }
  return year;
}

} // namespace

int64_t f$mktime(Optional<int64_t> hour, Optional<int64_t> minute, Optional<int64_t> second, Optional<int64_t> month, Optional<int64_t> day,
                 Optional<int64_t> year) noexcept {
  namespace chrono = std::chrono;
  const auto time_since_epoch{chrono::system_clock::now().time_since_epoch()};
  chrono::year_month_day current_date{chrono::sys_days{duration_cast<chrono::days>(time_since_epoch)}};

  const auto hours{chrono::hours(hour.has_value() ? hour.val() : duration_cast<chrono::hours>(time_since_epoch).count() % 24)};
  const auto minutes{chrono::minutes(minute.has_value() ? minute.val() : duration_cast<chrono::minutes>(time_since_epoch).count() % 60)};
  const auto seconds{chrono::seconds(second.has_value() ? second.val() : duration_cast<chrono::seconds>(time_since_epoch).count() % 60)};
  const auto months{chrono::months(month.has_value() ? month.val() : static_cast<unsigned>(current_date.month()))};
  const auto days{chrono::days(day.has_value() ? day.val() : static_cast<unsigned>(current_date.day()))};
  const auto years{chrono::years(year.has_value() ? fix_year(year.val()) : static_cast<int>(current_date.year()) - 1970)};

  const auto result{hours + minutes + seconds + months + days + years};
  return duration_cast<chrono::seconds>(result).count();
}

string f$gmdate(const string& format, Optional<int64_t> timestamp) noexcept {
  namespace chrono = std::chrono;
  const time_t now{timestamp.has_value() ? timestamp.val() : duration_cast<chrono::seconds>(chrono::system_clock::now().time_since_epoch()).count()};
  struct tm tm{};
  gmtime_r(std::addressof(now), std::addressof(tm));
  return date(format, tm, now, false);
}

string f$date(const string& format, Optional<int64_t> timestamp) noexcept {
  namespace chrono = std::chrono;
  const time_t now{timestamp.has_value() ? timestamp.val() : duration_cast<chrono::seconds>(chrono::system_clock::now().time_since_epoch()).count()};
  struct tm tm{};
  localtime_r(std::addressof(now), std::addressof(tm));
  return date(format, tm, now, true);
}
