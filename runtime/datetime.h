// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "runtime/kphp_core.h"

bool f$checkdate(int64_t month, int64_t day, int64_t year);

string f$date(const string &format, int64_t timestamp = std::numeric_limits<int64_t>::min());

bool f$date_default_timezone_set(const string &s);

string f$date_default_timezone_get();

array<mixed> f$getdate(int64_t timestamp = std::numeric_limits<int64_t>::min());

string f$gmdate(const string &format, int64_t timestamp = std::numeric_limits<int64_t>::min());

int64_t f$gmmktime(int64_t h = std::numeric_limits<int64_t>::min(),
                   int64_t m = std::numeric_limits<int64_t>::min(),
                   int64_t s = std::numeric_limits<int64_t>::min(),
                   int64_t month = std::numeric_limits<int64_t>::min(),
                   int64_t day = std::numeric_limits<int64_t>::min(),
                   int64_t year = std::numeric_limits<int64_t>::min());

array<mixed> f$localtime(int64_t timestamp = std::numeric_limits<int64_t>::min(), bool is_associative = false);

double microtime_monotonic();

double microtime();

mixed f$microtime(bool get_as_float = false);
string f$microtime_string();
double f$microtime_float();

int64_t f$mktime(int64_t h = std::numeric_limits<int64_t>::min(),
                 int64_t m = std::numeric_limits<int64_t>::min(),
                 int64_t s = std::numeric_limits<int64_t>::min(),
                 int64_t month = std::numeric_limits<int64_t>::min(),
                 int64_t day = std::numeric_limits<int64_t>::min(),
                 int64_t year = std::numeric_limits<int64_t>::min());

string f$strftime(const string &format, int64_t timestamp = std::numeric_limits<int64_t>::min());

Optional<int64_t> f$strtotime(const string &time_str, int64_t timestamp = std::numeric_limits<int64_t>::min());

int64_t f$time();

mixed f$hrtime(bool as_number = false);

void init_datetime_lib();
