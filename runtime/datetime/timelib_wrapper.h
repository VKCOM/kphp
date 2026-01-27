#pragma once

#include <string_view>

#include "runtime-common/core/runtime-core.h"
#include "runtime-common/stdlib/time/timelib-types.h"
#include "runtime/allocator.h"

// php_timelib wraps the https://github.com/derickr/timelib library
// which is used in PHP to implement several datetime lib functions.

using ScriptMemGuard = decltype(make_malloc_replacement_with_script_allocator());

void global_init_php_timelib();

std::pair<int64_t, bool> php_timelib_strtotime(const string& timezone, const string& time_str, int64_t timestamp);

array<mixed> php_timelib_date_parse(const string& time_str);
array<mixed> php_timelib_date_parse_from_format(const string& format, const string& time_str);

void free_timelib();

std::pair<kphp::timelib::time*, string> php_timelib_date_initialize(const string& tz_name, const string& time_str, const char* format = nullptr);
kphp::timelib::time* php_timelib_time_clone(kphp::timelib::time* t);
void php_timelib_date_remove(kphp::timelib::time* t);
Optional<array<mixed>> php_timelib_date_get_last_errors();

void php_timelib_date_timestamp_set(kphp::timelib::time* t, int64_t timestamp);
int64_t php_timelib_date_timestamp_get(kphp::timelib::time* t);
std::pair<bool, string> php_timelib_date_modify(kphp::timelib::time* t, const string& modifier);
void php_timelib_date_date_set(kphp::timelib::time* t, int64_t y, int64_t m, int64_t d);
void php_timelib_date_isodate_set(kphp::timelib::time* t, int64_t y, int64_t w, int64_t d);
void php_date_time_set(kphp::timelib::time* t, int64_t h, int64_t i, int64_t s, int64_t ms);
int64_t php_timelib_date_offset_get(kphp::timelib::time* t);

kphp::timelib::time* php_timelib_date_add(kphp::timelib::time* t, kphp::timelib::rel_time* interval);
std::pair<kphp::timelib::time*, std::string_view> php_timelib_date_sub(kphp::timelib::time* t, kphp::timelib::rel_time* interval);
kphp::timelib::rel_time* php_timelib_date_diff(kphp::timelib::time* time1, kphp::timelib::time* time2, bool absolute);

std::pair<kphp::timelib::rel_time*, string> php_timelib_date_interval_initialize(const string& format);
std::pair<kphp::timelib::rel_time*, string> php_timelib_date_interval_create_from_date_string(const string& time_str);
void php_timelib_date_interval_remove(kphp::timelib::rel_time* t);
