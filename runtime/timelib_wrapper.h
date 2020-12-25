#pragma once

#include "kphp_core.h"

// php_timelib wraps the https://github.com/derickr/timelib library
// which is used in PHP to implement several datetime lib functions.

const char* const PHP_TIMELIB_TZ_MOSCOW = "Europe/Moscow";
const char* const PHP_TIMELIB_TZ_GMT3 = "Etc/GMT-3";
const char* const PHP_TIMELIB_TZ_GMT4 = "Etc/GMT-4";

void php_timelib_init();

int php_timelib_days_in_month(int64_t month, int64_t year);

std::pair<int64_t, bool> php_timelib_strtotime(const string &timezone, const string &time_str, int64_t timestamp);

bool php_timelib_is_valid_date(int64_t month, int64_t day, int64_t year);
