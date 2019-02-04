#include "runtime/datetime.h"

#include <clocale>
#include <ctime>
#include <sys/time.h>

#include "runtime/string_functions.h"

extern long timezone;


static const char *day_of_week_names_short[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
static const char *day_of_week_names_full[] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};
static const char *suffix[] = {"st", "nd", "rd", "th"};
static const char *month_names_short[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
static const char *month_names_full[] = {"January", "February", "March", "April", "May", "June", "July", "August", "September", "October", "November",
                                         "December"};
static const int days_in_month[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

static time_t gmmktime(struct tm *tm) {
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

static inline int is_leap(int year) {
  return ((year % 4 == 0) ^ (year % 100 == 0) ^ (year % 400 == 0));
}

bool f$checkdate(int month, int day, int year) {
  return (1 <= month && month <= 12) &&
         (1 <= year && year <= 32767) &&
         (1 <= day && day <= days_in_month[month - 1] + (month == 2 && is_leap(year)));
}

static inline int fix_year(int year) {
  if ((unsigned int)year <= 100u) {
    if (year <= 69) {
      year += 2000;
    } else {
      year += 1900;
    }
  }
  return year;
}

static int month_by_full_name(const char *month_name) {
  for (int i = 0; i < 12; i++) {
    if (!strcmp(month_names_full[i], month_name)) {
      return i + 1;
    }
  }
  return 0;
}

static int day_of_week_by_full_name(const char *day_of_week_name) {
  for (int i = 0; i < 7; i++) {
    if (!strcmp(day_of_week_names_full[i], day_of_week_name)) {
      return i + 1;
    }
  }
  return 0;
}

void iso_week_number(int y, int doy, int weekday, int &iw, int &iy) {
  int y_leap, prev_y_leap, jan1weekday;

  y_leap = is_leap(y);
  prev_y_leap = is_leap(y - 1);

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


static string date(const string &format, const tm &t, int timestamp, bool local) {
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
  int internet_time;
  int iso_week, iso_year;

  SB.clean();
  for (int i = 0; i < (int)format.size(); i++) {
    switch (format[i]) {
      case 'd':
        SB << (char)(day / 10 + '0');
        SB << (char)(day % 10 + '0');
        break;
      case 'D':
        SB << day_of_week_names_short[day_of_week];
        break;
      case 'j':
        SB << day;
        break;
      case 'l':
        SB << day_of_week_names_full[day_of_week];
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
        SB << month_names_full[month - 1];
        break;
      case 'm':
        SB << (char)(month / 10 + '0');
        SB << (char)(month % 10 + '0');
        break;
      case 'M':
        SB << month_names_short[month - 1];
        break;
      case 'n':
        SB << month;
        break;
      case 't':
        SB << days_in_month[month - 1] + (month == 2 && is_leap(year));
        break;
      case 'L':
        SB << (int)is_leap(year);
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
          SB << "Europe/Moscow";
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
        SB << day_of_week_names_short[day_of_week];
        SB << ", ";
        SB << (char)(day / 10 + '0');
        SB << (char)(day % 10 + '0');
        SB << ' ';
        SB << month_names_short[month - 1];
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

string f$date(const string &format, int timestamp) {
  if (timestamp == INT_MIN) {
    timestamp = (int)time(nullptr);
  }
  tm t;
  time_t timestamp_t = timestamp;
  localtime_r(&timestamp_t, &t);

  return date(format, t, timestamp, true);
}

bool f$date_default_timezone_set(const string &s) {
  if (s != string("Etc/GMT-3", 9) && s != string("Europe/Moscow", 13)) {//TODO
    if (s == string("Etc/GMT-4", 9)) {
      php_warning("Timezone Etc/GMT-4 is not supported, use Etc/GMT-3 instead");
      return true;
    }
    php_critical_error ("unsupported default timezone \"%s\"", s.c_str());
  }
  return true;
}

string f$date_default_timezone_get() {
  return string("Europe/Moscow", 13);
}

array<var> f$getdate(int timestamp) {
  if (timestamp == INT_MIN) {
    timestamp = (int)time(nullptr);
  }
  tm t;
  time_t timestamp_t = timestamp;
  localtime_r(&timestamp_t, &t);

  array<var> result(array_size(1, 10, false));

  result.set_value(string("seconds", 7), t.tm_sec);
  result.set_value(string("minutes", 7), t.tm_min);
  result.set_value(string("hours", 5), t.tm_hour);
  result.set_value(string("mday", 4), t.tm_mday);
  result.set_value(string("wday", 4), t.tm_wday);
  result.set_value(string("mon", 3), t.tm_mon + 1);
  result.set_value(string("year", 4), t.tm_year + 1900);
  result.set_value(string("yday", 4), t.tm_yday);
  result.set_value(string("weekday", 7), string(day_of_week_names_full[t.tm_wday], (dl::size_type)strlen(day_of_week_names_full[t.tm_wday])));
  result.set_value(string("month", 5), string(month_names_full[t.tm_mon], (dl::size_type)strlen(month_names_full[t.tm_mon])));
  result.set_value(string("0", 1), timestamp);

  return result;
}

string f$gmdate(const string &format, int timestamp) {
  if (timestamp == INT_MIN) {
    timestamp = (int)time(nullptr);
  }
  tm t;
  time_t timestamp_t = timestamp;
  gmtime_r(&timestamp_t, &t);

  return date(format, t, timestamp, false);
}

int f$gmmktime(int h, int m, int s, int month, int day, int year) {
  tm t;
  time_t timestamp_t = time(nullptr);
  gmtime_r(&timestamp_t, &t);

  if (h != INT_MIN) {
    t.tm_hour = h;
  }

  if (m != INT_MIN) {
    t.tm_min = m;
  }

  if (s != INT_MIN) {
    t.tm_sec = s - (int)timezone;
  }

  if (month != INT_MIN) {
    t.tm_mon = month - 1;
  }

  if (day != INT_MIN) {
    t.tm_mday = day;
  }

  if (year != INT_MIN) {
    t.tm_year = fix_year(year) - 1900;
  }

  t.tm_isdst = -1;
  return (int)gmmktime(&t) - 3 * 3600;
}

array<var> f$localtime(int timestamp, bool is_associative) {
  if (timestamp == INT_MIN) {
    timestamp = (int)time(nullptr);
  }
  tm t;
  time_t timestamp_t = timestamp;
  localtime_r(&timestamp_t, &t);

  if (!is_associative) {
    return array<var>::create(t.tm_sec, t.tm_min, t.tm_hour, t.tm_mday, t.tm_mon, t.tm_year, t.tm_wday, t.tm_yday, t.tm_isdst);
  }

  array<var> result(array_size(0, 9, false));

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


double microtime_monotonic() {
  struct timespec T;
  php_assert (clock_gettime(CLOCK_MONOTONIC, &T) >= 0);
  return (double)T.tv_sec + T.tv_nsec * 1e-9;
}

static string microtime_string() {
  struct timespec T;
  php_assert (clock_gettime(CLOCK_REALTIME, &T) >= 0);
  char buf[45];
  int len = sprintf(buf, "0.%09d %d", (int)T.tv_nsec, (int)T.tv_sec);
  return string(buf, len);
}

double microtime() {
  struct timespec T;
  php_assert (clock_gettime(CLOCK_REALTIME, &T) >= 0);
  return (double)T.tv_sec + T.tv_nsec * 1e-9;
}

var f$microtime(bool get_as_float) {
  if (get_as_float) {
    return microtime();
  } else {
    return microtime_string();
  }
}

int f$mktime(int h, int m, int s, int month, int day, int year) {
  tm t;
  time_t timestamp_t = time(nullptr);
  localtime_r(&timestamp_t, &t);

  if (h != INT_MIN) {
    t.tm_hour = h;
  }

  if (m != INT_MIN) {
    t.tm_min = m;
  }

  if (s != INT_MIN) {
    t.tm_sec = s;
  }

  if (month != INT_MIN) {
    t.tm_mon = month - 1;
  }

  if (day != INT_MIN) {
    t.tm_mday = day;
  }

  if (year != INT_MIN) {
    t.tm_year = fix_year(year) - 1900;
  }

  t.tm_isdst = -1;

  return (int)mktime(&t);
}

string f$strftime(const string &format, int timestamp) {
  if (timestamp == INT_MIN) {
    timestamp = (int)time(nullptr);
  }
  tm t;
  time_t timestamp_t = timestamp;
  localtime_r(&timestamp_t, &t);

  if (!strftime(php_buf, PHP_BUF_LEN, format.c_str(), &t)) {
    return string();
  }

  return string(php_buf, (dl::size_type)strlen(php_buf));
}

OrFalse<int> f$strtotime(const string &time_str, int timestamp) {
  if (timestamp == INT_MIN) {
    timestamp = (int)time(nullptr);
  }
  tm t;
  time_t timestamp_t = timestamp;
  localtime_r(&timestamp_t, &t);

  string s = f$trim(time_str);

  char str[21];
  bool time_set = false;

  int old_size;
  do {
    old_size = s.size();

    int d, m, y, pos = -1;
    if ((sscanf(s.c_str(), "%d.%d.%d %n", &d, &m, &y, &pos) == 3 ||
         sscanf(s.c_str(), "%d-%d-%d %n", &y, &m, &d, &pos) == 3 ||
         sscanf(s.c_str(), "%4d%2d%2d %n", &y, &m, &d, &pos) == 3 ||
         (sscanf(s.c_str(), "%d %20s %d %n", &d, str, &y, &pos) == 3 && (m = month_by_full_name(str))) ||
         (sscanf(s.c_str(), "%d %20s %n", &d, str, &pos) == 2 && (m = month_by_full_name(str)) && (y = t.tm_year + 1900))) &&
        pos != -1) {
      t.tm_mday = d;
      t.tm_mon = m - 1;
      t.tm_year = fix_year(y) - 1900;

      if (!time_set) {
        t.tm_sec = 0;
        t.tm_min = 0;
        t.tm_hour = 0;
      }

      s = s.substr(pos, s.size() - pos);
    }

    if (((pos = 8) > 0 && !strncmp(s.c_str(), "tomorrow", pos)) ||
        ((pos = 5) > 0 && !strncmp(s.c_str(), "today", pos)) ||
        ((pos = 8) > 0 && !strncmp(s.c_str(), "midnight", pos))) {
      t.tm_mday += (s[3] == 'o' || s[3] == 't');

      if (!time_set) {
        t.tm_sec = 0;
        t.tm_min = 0;
        t.tm_hour = 0;
      }

      while (s[pos] == ' ') {
        pos++;
      }
      s = s.substr(pos, s.size() - pos);
    }

    if (!strncmp(s.c_str(), "now", 3)) {
      pos = 3;
      while (s[pos] == ' ') {
        pos++;
      }
      s = s.substr(pos, s.size() - pos);
    }

    if (!strncmp(s.c_str(), "next day", 8)) {
      t.tm_mday++;
      pos = 8;
      while (s[pos] == ' ') {
        pos++;
      }
      s = s.substr(pos, s.size() - pos);
    }

    if (!strncmp(s.c_str(), "next month", 10)) {
      t.tm_mon++;
      pos = 10;
      while (s[pos] == ' ') {
        pos++;
      }
      s = s.substr(pos, s.size() - pos);
    }

    pos = -1;
    if (!strncmp(s.c_str(), "next ", 5) && sscanf(s.c_str() + 5, "%20s %n", str, &pos) == 1 && (d = day_of_week_by_full_name(str)) > 0 && pos != -1) {
      if (d > t.tm_wday) {
        t.tm_mday += d - 1 - t.tm_wday;
      } else {
        t.tm_mday += d + 6 - t.tm_wday;
      }

      if (!time_set) {
        t.tm_sec = 0;
        t.tm_min = 0;
        t.tm_hour = 0;
      }

      s = s.substr(5 + pos, s.size() - pos - 5);
    }

    int ho, mi, se;
    pos = -1;
    if ((sscanf(s.c_str(), "%d:%d:%d %n", &ho, &mi, &se, &pos) == 3 ||
         (sscanf(s.c_str(), "%d:%d %n", &ho, &mi, &pos) == 2 && (se = 0) == 0)) &&
        pos != -1) {
      t.tm_sec = se;
      t.tm_min = mi;
      t.tm_hour = ho;

      time_set = true;

      s = s.substr(pos, s.size() - pos);
    }

    if (s[0] == '-' || (s[0] == '+' || ('0' <= s[0] && s[0] <= '9'))) {
      int cnt;
      pos = -1;
      if (sscanf(s.c_str(), "%d%20s %n", &cnt, str, &pos) == 2 && pos != -1) {
        bool error = false;
        switch (str[0]) {
          case 'd':
            if (str[1] == 'a' && str[2] == 'y' && ((str[3] == 's' && str[4] == 0) || str[3] == 0)) {
              t.tm_mday += cnt;
              break;
            }
            error = true;
            break;
          case 'h':
            if (str[1] == 'o' && str[2] == 'u' && str[3] == 'r' && ((str[4] == 's' && str[5] == 0) || str[4] == 0)) {
              t.tm_hour += cnt;
              break;
            }
            error = true;
            break;
          case 'm':
            if (str[1] == 'o' && str[2] == 'n' && str[3] == 't' && str[4] == 'h' && ((str[5] == 's' && str[6] == 0) || str[5] == 0)) {
              t.tm_mon += cnt;
              break;
            }
            if (str[1] == 'i' && str[2] == 'n' && str[3] == 'u' && str[4] == 't' && str[5] == 'e' && ((str[6] == 's' && str[7] == 0) || str[6] == 0)) {
              t.tm_min += cnt;
              break;
            }
            error = true;
            break;
          case 'w':
            if (str[1] == 'e' && str[2] == 'e' && str[3] == 'k' && ((str[4] == 's' && str[5] == 0) || str[4] == 0)) {
              t.tm_mday += cnt * 7;
              if (!strncmp(s.c_str() + pos, "1 day", 5)) {
                t.tm_mday += (s[0] == '+' ? 1 : -1);
                pos += 5;
                while (s[pos] == ' ') {
                  pos++;
                }
              }
              break;
            }
            error = true;
            break;
          case 'y':
            if (str[1] == 'e' && str[2] == 'a' && str[3] == 'r' && ((str[4] == 's' && str[5] == 0) || str[4] == 0)) {
              t.tm_year += cnt;
              break;
            }
            error = true;
            break;
          default:
            error = true;
            break;
        }

        if (!error) {
          s = s.substr(pos, s.size() - pos);
        }
      }
    }
  } while (old_size != (int)s.size() && s.size());

  bool need_gmt = false;
  if ((int)s.size() != 0) {
    const char *patterns[] = {"%c", "%Ec", "%FT%T", "%x", "%Ex", "%X", "%EX", "%A, %B %d, %Y %I:%M:%S %p", "%A, %d %b %Y %T"};
    int patterns_size = sizeof(patterns) / sizeof(patterns[0]);

    bool found = false;
    string cur_locale;

    for (int tr = 0; tr < 2 && !found; tr++) {
      for (int i = 0; i < patterns_size && !found; i++) {
        const char *res = strptime(s.c_str(), patterns[i], &t);
        if (res != nullptr) {
          while (*res == ' ') {
            res++;
          }
//          fprintf (stderr, "%d %d: %s !!! \"%s\"\n", tr, i, patterns[i], res);
          if (*res == 0 || !strcmp("MSK", res)) {
            found = true;
          } else if (!strcmp("GMT", res)) {
            need_gmt = true;
//            t.tm_hour -= 3;
            found = true;
          } else if (*res == '-' || *res == '+') {
            int val = 0;
            int mul = (*res++ == '+') * 2 - 1;
            int n;
            for (n = 0; n < 4 && *res >= '0' && *res <= '9'; n++) {
              val = val * 10 + (*res++ - '0');
            }
            if (n == 2) {
              val *= 100;
              n = 4;
            } else if (n == 4 && val % 100 < 60) {
              val = (val / 100) * 100 + ((val % 100) * 50) / 30;
            }
            if (n == 4 && val <= 1200) {
              t.tm_sec -= mul * val * 36;
              need_gmt = true;
              found = true;
            }
          }
        }
      }

      if (tr == 0) {
        if (!found) {
          const char *cur_locale_c_ctr = setlocale(LC_TIME, nullptr);
          php_assert (cur_locale_c_ctr != nullptr);
          cur_locale.assign(cur_locale_c_ctr, (dl::size_type)strlen(cur_locale_c_ctr));
          setlocale(LC_TIME, "C");
        }
      } else {
        setlocale(LC_TIME, cur_locale.c_str());
      }
    }

    if (found) {
      s = string();
    }
  }

  if ((int)s.size() == 0) {
    t.tm_isdst = -1;
    return need_gmt ? (int)gmmktime(&t) : (int)mktime(&t);
  }

  php_critical_error ("strtotime can't parse string \"%s\", unparsed part: \"%s\"", time_str.c_str(), s.c_str());
  return false;
}

int f$time() {
  return (int)time(nullptr);
}


void datetime_init_static() {
  dl::enter_critical_section();//OK

  setenv("TZ", "Etc/GMT-3", 1);
  tzset();

  dl::leave_critical_section();
}
