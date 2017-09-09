#pragma once

#include <climits>

#include "runtime/kphp_core.h"
#include "runtime/string_functions.h"

bool mb_UTF8_check (const char *s);

bool f$mb_check_encoding (const string &str, const string &encoding = CP1251);

int f$mb_strlen (const string &str, const string &encoding = CP1251);

string f$mb_strtolower (const string &str, const string &encoding = CP1251);

string f$mb_strtoupper (const string &str, const string &encoding = CP1251);

OrFalse <int> f$mb_strpos (const string &haystack, const string &needle, int offset = 0, const string &encoding = CP1251);

OrFalse <int> f$mb_stripos (const string &haystack, const string &needle, int offset = 0, const string &encoding = CP1251);

string f$mb_substr (const string &str, int start, int length = INT_MAX, const string &encoding = CP1251);
