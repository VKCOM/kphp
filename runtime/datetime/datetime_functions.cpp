// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime/datetime/datetime_functions.h"

#include <clocale>
#include <ctime>
#include <sys/time.h>
#include <chrono>

#include "runtime/critical_section.h"
#include "runtime/datetime/timelib_wrapper.h"
#include "runtime/string_functions.h"
#include "server/server-log.h"

extern long timezone;

static const char *default_timezone_id = "";

// this function should only be called with supported timezone_id values;
// it could be "Etc/GMT-3" or "Europe/Moscow"
static void set_default_timezone_id(const char *timezone_id) {
  if (strcmp(default_timezone_id, timezone_id) == 0) {
    return;
  }
  {
    dl::CriticalSectionGuard critical_section;
    setenv("TZ", timezone_id, 1);
    tzset();
  }
  default_timezone_id = timezone_id;
}

static const char *suffix[] = {"st", "nd", "rd", "th"};

static bool use_updated_gmmktime = false;

static time_t deprecated_gmmktime(struct tm * tm) {
  char *tz = getenv("TZ");
  setenv("TZ", "", 1);
  tzset();

  time_t result = mktime(tm);

  if (tz) {
    setenv("TZ", tz, 1);
  } else {
    unsetenv("TZ");
  }
  tzset();

  return result;
}

bool f$checkdate(int64_t month, int64_t day, int64_t year) {
  return php_timelib_is_valid_date(month, day, year);
}

static inline int32_t fix_year(int32_t year) {
  if ((uint64_t)year <= 100u) {
    if (year <= 69) {
      year += 2000;
    } else {
      year += 1900;
    }
  }
  return year;
}

void iso_week_number(int y, int doy, int weekday, int &iw, int &iy) {
  int y_leap, prev_y_leap, jan1weekday;

  y_leap = timelib_is_leap_year(y);
  prev_y_leap = timelib_is_leap_year(y - 1);

  jan1weekday = (weekday - (doy % 7) + 7) % 7;

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
    int i;
    i = y_leap ? 366 : 365;
    if ((i - (doy - y_leap + 1)) < (4 - weekday)) {
      iy = y + 1;
      iw = 1;
      return;
    }
  }
  /* Find if Y M D falls in YearNumber Y, WeekNumber 1 through 53 */
  if (iy == y) {
    int j;
    j = doy + (7 - weekday) + jan1weekday;
    iw = j / 7;
    if (jan1weekday > 4) {
      iw -= 1;
    }
  }
}


static string date(const string &format, const tm &t, int64_t timestamp, bool local) {
  string_buffer &SB = static_SB_spare;

  int year = t.tm_year + 1900;
  int month = t.tm_mon + 1;
  int day = t.tm_mday;
  int hour = t.tm_hour;
  int hour12 = (hour + 11) % 12 + 1;
  int minute = t.tm_min;
  int second = t.tm_sec;
  int day_of_week = t.tm_wday;
  int day_of_year = t.tm_yday;
  int64_t internet_time;
  int iso_week, iso_year;

  SB.clean();
  for (int i = 0; i < (int)format.size(); i++) {
    switch (format[i]) {
      case 'd':
        SB << (char)(day / 10 + '0');
        SB << (char)(day % 10 + '0');
        break;
      case 'D':
        SB << PHP_TIMELIB_DAY_SHORT_NAMES[day_of_week];
        break;
      case 'j':
        SB << day;
        break;
      case 'l':
        SB << PHP_TIMELIB_DAY_FULL_NAMES[day_of_week];
        break;
      case 'N':
        SB << (day_of_week == 0 ? '7' : (char)(day_of_week + '0'));
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
        SB << suffix[c];
        break;
      }
      case 'w':
        SB << (char)(day_of_week + '0');
        break;
      case 'z':
        SB << day_of_year;
        break;
      case 'W':
        iso_week_number(year, day_of_year, day_of_week, iso_week, iso_year);
        SB << (char)('0' + iso_week / 10);
        SB << (char)('0' + iso_week % 10);
        break;
      case 'F':
        SB << PHP_TIMELIB_MON_FULL_NAMES[month - 1];
        break;
      case 'm':
        SB << (char)(month / 10 + '0');
        SB << (char)(month % 10 + '0');
        break;
      case 'M':
        SB << PHP_TIMELIB_MON_SHORT_NAMES[month - 1];
        break;
      case 'n':
        SB << month;
        break;
      case 't':
        SB << php_timelib_days_in_month(month, year);
        break;
      case 'L':
        SB << (int)timelib_is_leap_year(year);
        break;
      case 'o':
        iso_week_number(year, day_of_year, day_of_week, iso_week, iso_year);
        SB << iso_year;
        break;
      case 'Y':
        SB << year;
        break;
      case 'y':
        SB << (char)(year / 10 % 10 + '0');
        SB << (char)(year % 10 + '0');
        break;
      case 'a':
        SB << (hour < 12 ? "am" : "pm");
        break;
      case 'A':
        SB << (hour < 12 ? "AM" : "PM");
        break;
      case 'B':
        internet_time = (timestamp + 3600) % 86400 * 1000 / 86400;
        SB << (char)(internet_time / 100 + '0');
        SB << (char)((internet_time / 10) % 10 + '0');
        SB << (char)(internet_time % 10 + '0');
        break;
      case 'g':
        SB << hour12;
        break;
      case 'G':
        SB << hour;
        break;
      case 'h':
        SB << (char)(hour12 / 10 + '0');
        SB << (char)(hour12 % 10 + '0');
        break;
      case 'H':
        SB << (char)(hour / 10 + '0');
        SB << (char)(hour % 10 + '0');
        break;
      case 'i':
        SB << (char)(minute / 10 + '0');
        SB << (char)(minute % 10 + '0');
        break;
      case 's':
        SB << (char)(second / 10 + '0');
        SB << (char)(second % 10 + '0');
        break;
      case 'u':
        SB << "000000";
        break;
      case 'e':
        if (local) {
          SB << PHP_TIMELIB_TZ_MOSCOW;
        } else {
          SB << "UTC";
        }
        break;
      case 'I':
        SB << (int)(t.tm_isdst > 0);
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
        SB << (char)(month / 10 + '0');
        SB << (char)(month % 10 + '0');
        SB << '-';
        SB << (char)(day / 10 + '0');
        SB << (char)(day % 10 + '0');
        SB << "T";
        SB << (char)(hour / 10 + '0');
        SB << (char)(hour % 10 + '0');
        SB << ':';
        SB << (char)(minute / 10 + '0');
        SB << (char)(minute % 10 + '0');
        SB << ':';
        SB << (char)(second / 10 + '0');
        SB << (char)(second % 10 + '0');
        if (local) {
          SB << "+03:00";
        } else {
          SB << "+00:00";
        }
        break;
      case 'r':
        SB << PHP_TIMELIB_DAY_SHORT_NAMES[day_of_week];
        SB << ", ";
        SB << (char)(day / 10 + '0');
        SB << (char)(day % 10 + '0');
        SB << ' ';
        SB << PHP_TIMELIB_MON_SHORT_NAMES[month - 1];
        SB << ' ';
        SB << year;
        SB << ' ';
        SB << (char)(hour / 10 + '0');
        SB << (char)(hour % 10 + '0');
        SB << ':';
        SB << (char)(minute / 10 + '0');
        SB << (char)(minute % 10 + '0');
        SB << ':';
        SB << (char)(second / 10 + '0');
        SB << (char)(second % 10 + '0');
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

string f$date(const string &format, int64_t timestamp) {
  if (timestamp == std::numeric_limits<int64_t>::min()) {
    timestamp = time(nullptr);
  }
  tm t;
  time_t timestamp_t = timestamp;
  localtime_r(&timestamp_t, &t);

  return date(format, t, timestamp, true);
}

bool f$date_default_timezone_set(const string &s) {
  if (strcmp(s.c_str(), PHP_TIMELIB_TZ_GMT3) == 0) {
    set_default_timezone_id(PHP_TIMELIB_TZ_GMT3);
    return true;
  }
  if (strcmp(s.c_str(), PHP_TIMELIB_TZ_MOSCOW) == 0) {
    set_default_timezone_id(PHP_TIMELIB_TZ_MOSCOW);
    return true;
  }
  // TODO: this branch is weird; remove it?
  if (strcmp(s.c_str(), PHP_TIMELIB_TZ_GMT4) == 0) {
    php_warning("Timezone %s is not supported, use %s instead", PHP_TIMELIB_TZ_GMT4, PHP_TIMELIB_TZ_GMT3);
    return false;
  }
  php_critical_error ("unsupported default timezone \"%s\"", s.c_str());
}

string f$date_default_timezone_get() {
  return string(default_timezone_id);
}

array<mixed> f$getdate(int64_t timestamp) {
  if (timestamp == std::numeric_limits<int64_t>::min()) {
    timestamp = time(nullptr);
  }
  tm t;
  time_t timestamp_t = timestamp;
  localtime_r(&timestamp_t, &t);

  array<mixed> result(array_size(1, 10, false));

  result.set_value(string("seconds", 7), t.tm_sec);
  result.set_value(string("minutes", 7), t.tm_min);
  result.set_value(string("hours", 5), t.tm_hour);
  result.set_value(string("mday", 4), t.tm_mday);
  result.set_value(string("wday", 4), t.tm_wday);
  result.set_value(string("mon", 3), t.tm_mon + 1);
  result.set_value(string("year", 4), t.tm_year + 1900);
  result.set_value(string("yday", 4), t.tm_yday);
  result.set_value(string("weekday", 7), string(PHP_TIMELIB_DAY_FULL_NAMES[t.tm_wday]));
  result.set_value(string("month", 5), string(PHP_TIMELIB_MON_FULL_NAMES[t.tm_mon]));
  result.set_value(string("0", 1), timestamp);

  return result;
}

void f$set_use_updated_gmmktime(bool enable) {
  use_updated_gmmktime = enable;
}

string f$gmdate(const string &format, int64_t timestamp) {
  if (timestamp == std::numeric_limits<int64_t>::min()) {
    timestamp = time(nullptr);
  }
  tm t;
  time_t timestamp_t = timestamp;
  gmtime_r(&timestamp_t, &t);

  return date(format, t, timestamp, false);
}

int64_t f$gmmktime(int64_t h, int64_t m, int64_t s, int64_t month, int64_t day, int64_t year) {
  dl::CriticalSectionGuard guard;
  tm t;
  time_t timestamp_t = time(nullptr);
  gmtime_r(&timestamp_t, &t);

  if (h != std::numeric_limits<int64_t>::min()) {
    t.tm_hour = static_cast<int32_t>(h);
  }

  if (m != std::numeric_limits<int64_t>::min()) {
    t.tm_min = static_cast<int32_t>(m);
  }

  if (s != std::numeric_limits<int64_t>::min()) {
    if (use_updated_gmmktime) {
      t.tm_sec = static_cast<int32_t>(s);
    } else {
      t.tm_sec = static_cast<int32_t>(s - timezone);
    }
  }

  if (month != std::numeric_limits<int64_t>::min()) {
    t.tm_mon = static_cast<int32_t>(month - 1);
  }

  if (day != std::numeric_limits<int64_t>::min()) {
    t.tm_mday = static_cast<int32_t>(day);
  }

  if (year != std::numeric_limits<int64_t>::min()) {
    t.tm_year = fix_year(static_cast<int32_t>(year)) - 1900;
  }

  t.tm_isdst = -1;

  if (use_updated_gmmktime) {
    return timegm(&t);
  } else {
    return deprecated_gmmktime(&t) - 3 * 3600;
  }
}

array<mixed> f$localtime(int64_t timestamp, bool is_associative) {
  if (timestamp == std::numeric_limits<int64_t>::min()) {
    timestamp = time(nullptr);
  }
  tm t;
  time_t timestamp_t = timestamp;
  localtime_r(&timestamp_t, &t);

  if (!is_associative) {
    return array<mixed>::create(t.tm_sec, t.tm_min, t.tm_hour, t.tm_mday, t.tm_mon, t.tm_year, t.tm_wday, t.tm_yday, t.tm_isdst);
  }

  array<mixed> result(array_size(0, 9, false));

  result.set_value(string("tm_sec", 6), t.tm_sec);
  result.set_value(string("tm_min", 6), t.tm_min);
  result.set_value(string("tm_hour", 7), t.tm_hour);
  result.set_value(string("tm_mday", 7), t.tm_mday);
  result.set_value(string("tm_mon", 6), t.tm_mon);
  result.set_value(string("tm_year", 7), t.tm_year);
  result.set_value(string("tm_wday", 7), t.tm_wday);
  result.set_value(string("tm_yday", 7), t.tm_yday);
  result.set_value(string("tm_isdst", 8), t.tm_isdst);

  return result;
}

void free_use_updated_gmmktime() {
  use_updated_gmmktime = false;
}


double microtime_monotonic() {
  struct timespec T;
  php_assert (clock_gettime(CLOCK_MONOTONIC, &T) >= 0);
  return static_cast<double>(T.tv_sec) + static_cast<double>(T.tv_nsec) * 1e-9;
}

static string microtime_string() {
  struct timespec T;
  php_assert (clock_gettime(CLOCK_REALTIME, &T) >= 0);
  const size_t buf_size = 45;
  char buf[buf_size];
  int len = snprintf(buf, buf_size, "0.%09d %d", (int)T.tv_nsec, (int)T.tv_sec);
  return {buf, static_cast<string::size_type>(len)};
}

double microtime() {
  struct timespec T;
  php_assert (clock_gettime(CLOCK_REALTIME, &T) >= 0);
  return static_cast<double>(T.tv_sec) + static_cast<double>(T.tv_nsec) * 1e-9;
}

mixed f$microtime(bool get_as_float) {
  if (get_as_float) {
    return microtime();
  } else {
    return microtime_string();
  }
}

double f$_microtime_float() { return microtime(); }
string f$_microtime_string() { return microtime_string(); }

int64_t f$mktime(int64_t h, int64_t m, int64_t s, int64_t month, int64_t day, int64_t year) {
  tm t;
  time_t timestamp_t = time(nullptr);
  localtime_r(&timestamp_t, &t);

  if (h != std::numeric_limits<int64_t>::min()) {
    t.tm_hour = static_cast<int32_t>(h);
  }

  if (m != std::numeric_limits<int64_t>::min()) {
    t.tm_min = static_cast<int32_t>(m);
  }

  if (s != std::numeric_limits<int64_t>::min()) {
    t.tm_sec = static_cast<int32_t>(s);
  }

  if (month != std::numeric_limits<int64_t>::min()) {
    t.tm_mon = static_cast<int32_t>(month - 1);
  }

  if (day != std::numeric_limits<int64_t>::min()) {
    t.tm_mday = static_cast<int32_t>(day);
  }

  if (year != std::numeric_limits<int64_t>::min()) {
    t.tm_year = fix_year(static_cast<int32_t>(year)) - 1900;
  }

  t.tm_isdst = -1;

  return mktime(&t);
}

string f$strftime(const string &format, int64_t timestamp) {
  if (timestamp == std::numeric_limits<int64_t>::min()) {
    timestamp = time(nullptr);
  }
  tm t;
  time_t timestamp_t = timestamp;
  localtime_r(&timestamp_t, &t);

  if (!strftime(php_buf, PHP_BUF_LEN, format.c_str(), &t)) {
    return {};
  }

  return string(php_buf);
}

Optional<int64_t> f$strtotime(const string &time_str, int64_t timestamp) {
  if (timestamp == std::numeric_limits<int64_t>::min()) {
    timestamp = time(nullptr);
  }
  auto [ts, ok] = php_timelib_strtotime(f$date_default_timezone_get(), time_str, timestamp);
  return ok ? Optional<int64_t>(ts) : Optional<int64_t>(false);
}

array<mixed> f$date_parse(const string &time_str) {
  return php_timelib_date_parse(time_str);
}

array<mixed> f$date_parse_from_format(const string &format, const string &time_str) {
  return php_timelib_date_parse_from_format(format, time_str);
}

int64_t f$time() {
  return time(nullptr);
}

int64_t hrtime_int() {
  return std::chrono::steady_clock::now().time_since_epoch().count();
}

array<int64_t> hrtime_array() {
  auto since_epoch = std::chrono::steady_clock::now().time_since_epoch();
  return array<int64_t>::create(
    std::chrono::duration_cast<std::chrono::seconds>(since_epoch).count(),
    std::chrono::nanoseconds{since_epoch % std::chrono::seconds{1}}.count()
  );
}

mixed f$hrtime(bool as_number) {
  if (as_number) {
    return hrtime_int();
  }
  return hrtime_array();
}

array<int64_t> f$_hrtime_array() { return hrtime_array(); }
int64_t f$_hrtime_int() { return hrtime_int(); }

void init_datetime_lib() {
  dl::enter_critical_section();//OK

  set_default_timezone_id(PHP_TIMELIB_TZ_MOSCOW);

  dl::leave_critical_section();
}
