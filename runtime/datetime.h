#pragma once

#include "kphp_core.h"

bool f$checkdate (int month, int day, int year);

string f$date (const string &format, int timestamp = INT_MIN);

bool f$date_default_timezone_set (const string &s);

string f$date_default_timezone_get (void);

array <var> f$getdate (int timestamp = INT_MIN);

string f$gmdate (const string &format, int timestamp = INT_MIN);

int f$gmmktime (int h = INT_MIN, int m = INT_MIN, int s = INT_MIN, int month = INT_MIN, int day = INT_MIN, int year = INT_MIN);

array <var> f$localtime (int timestamp = INT_MIN, bool is_associative = false);

string microtime (void);

double microtime (bool get_as_float);

var f$microtime (bool get_as_float = false);

int f$mktime (int h = INT_MIN, int m = INT_MIN, int s = INT_MIN, int month = INT_MIN, int day = INT_MIN, int year = INT_MIN);

string f$strftime (const string &format, int timestamp = INT_MIN);

OrFalse <int> f$strtotime (const string &time_str, int timestamp = INT_MIN);

int f$time (void);


void datetime_init_static (void);