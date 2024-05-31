#pragma once

#include <string_view>

#include "runtime-core/runtime-core.h"

// php_timelib wraps the https://github.com/derickr/timelib library
// which is used in PHP to implement several datetime lib functions.

const char *const PHP_TIMELIB_TZ_MOSCOW = "Europe/Moscow";
const char *const PHP_TIMELIB_TZ_GMT3 = "Etc/GMT-3";
const char *const PHP_TIMELIB_TZ_GMT4 = "Etc/GMT-4";

const char *const PHP_TIMELIB_MON_FULL_NAMES[] = {"January", "February", "March",     "April",   "May",      "June",
                                                  "July",    "August",   "September", "October", "November", "December"};
const char *const PHP_TIMELIB_MON_SHORT_NAMES[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
const char *const PHP_TIMELIB_DAY_FULL_NAMES[] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};
const char *const PHP_TIMELIB_DAY_SHORT_NAMES[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};

void global_init_php_timelib();

int php_timelib_days_in_month(int64_t month, int64_t year);

std::pair<int64_t, bool> php_timelib_strtotime(const string &timezone, const string &time_str, int64_t timestamp);

array<mixed> php_timelib_date_parse(const string &time_str);
array<mixed> php_timelib_date_parse_from_format(const string &format, const string &time_str);

bool php_timelib_is_valid_date(int64_t month, int64_t day, int64_t year);

void free_timelib();

struct _timelib_time;
using timelib_time = _timelib_time;

struct _timelib_rel_time;
using timelib_rel_time = _timelib_rel_time;

std::pair<timelib_time *, string> php_timelib_date_initialize(const string &tz_name, const string &time_str, const char *format = nullptr);
timelib_time *php_timelib_time_clone(timelib_time *t);
void php_timelib_date_remove(timelib_time *t);
Optional<array<mixed>> php_timelib_date_get_last_errors();

constexpr bool timelib_is_leap_year(int32_t year) noexcept {
  return (year % 4 == 0) && (year % 100 != 0 || year % 400 == 0);
}

string php_timelib_date_format(const string &format, timelib_time *t, bool localtime);
string php_timelib_date_format_localtime(const string &format, timelib_time *t);
void php_timelib_date_timestamp_set(timelib_time *t, int64_t timestamp);
int64_t php_timelib_date_timestamp_get(timelib_time *t);
std::pair<bool, string> php_timelib_date_modify(timelib_time *t, const string &modifier);
void php_timelib_date_date_set(timelib_time *t, int64_t y, int64_t m, int64_t d);
void php_timelib_date_isodate_set(timelib_time *t, int64_t y, int64_t w, int64_t d);
void php_date_time_set(timelib_time *t, int64_t h, int64_t i, int64_t s, int64_t ms);
int64_t php_timelib_date_offset_get(timelib_time *t);

timelib_time *php_timelib_date_add(timelib_time *t, timelib_rel_time *interval);
std::pair<timelib_time *, std::string_view> php_timelib_date_sub(timelib_time *t, timelib_rel_time *interval);
timelib_rel_time *php_timelib_date_diff(timelib_time *time1, timelib_time *time2, bool absolute);

std::pair<timelib_rel_time *, string> php_timelib_date_interval_initialize(const string &format);
std::pair<timelib_rel_time *, string> php_timelib_date_interval_create_from_date_string(const string &time_str);
void php_timelib_date_interval_remove(timelib_rel_time *t);
string php_timelib_date_interval_format(const string &format, timelib_rel_time *t);
