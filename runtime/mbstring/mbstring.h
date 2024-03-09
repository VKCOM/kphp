#pragma once

#include "runtime/kphp_core.h"
#include "common/type_traits/function_traits.h"
#include "common/vector-product.h"

#include "runtime/kphp_core.h"
#include "runtime/math_functions.h"
#include "runtime/string_functions.h"

bool mb_UTF8_check(const char *s);

#ifdef MBFL


bool f$mb_check_encoding(const mixed &value, const Optional<string> &encoding);

mixed f$mb_convert_encoding(const mixed &str, const string &to_encoding, const mixed &from_encoding);

array<string> f$mb_str_split(const string &str, const int64_t &length=1, const Optional<string> &encoding=false);

int64_t f$mb_strlen(const string &str, const Optional<string> &encoding=false);

string f$mb_strcut(const string &str, const int64_t start, const Optional<int64_t> &length=0, const Optional<string> &encoding=false);

string f$mb_substr(const string &str, const int64_t start, const Optional<int64_t> &length=0, const Optional<string> &encoding=false);

int64_t f$mb_substr_count(const string &haystack, const string &needle, const Optional<string> &encoding=false);

string f$mb_strtoupper(const string &str, const Optional<string> &encoding=false);

string f$mb_strtolower(const string &str, const Optional<string> &encoding=false);

int64_t f$mb_strwidth(const string &str, const Optional<string> &encoding=false);

Optional<int64_t> f$mb_strpos(const string &haystack, const string &needle, const int64_t offset, const Optional<string> &encoding=false);

Optional<int64_t> f$mb_strrpos(const string &haystack, const string &needle, const int64_t offset, const Optional<string> &encoding=false);

Optional<int64_t> f$mb_strripos(const string &haystack, const string &needle, const int64_t offset, const Optional<string> &encoding=false);

Optional<int64_t> f$mb_stripos(const string &haystack, const string &needle, const int64_t offset, const Optional<string> &encoding=false);

Optional<string> f$mb_stristr(const string &haystack, const string &needle, const bool before_needle, const Optional<string> &encoding=false);

Optional<string> f$mb_strstr(const string &haystack, const string &needle, const bool before_needle, const Optional<string> &encoding=false);

Optional<string> f$mb_strrchr(const string &haystack, const string &needle, const bool before_needle, const Optional<string> &encoding=false);

Optional<string> f$mb_strrichr(const string &haystack, const string &needle, const bool before_needle, const Optional<string> &encoding=false);

#else

#include <climits>

#include "runtime/kphp_core.h"
#include "runtime/string_functions.h"

bool f$mb_check_encoding(const string &str, const string &encoding = CP1251);

int64_t f$mb_strlen(const string &str, const string &encoding = CP1251);

string f$mb_strtolower(const string &str, const string &encoding = CP1251);

string f$mb_strtoupper(const string &str, const string &encoding = CP1251);

Optional<int64_t> f$mb_strpos(const string &haystack, const string &needle, int64_t offset = 0, const string &encoding = CP1251) noexcept;

Optional<int64_t> f$mb_stripos(const string &haystack, const string &needle, int64_t offset = 0, const string &encoding = CP1251) noexcept;

string f$mb_substr(const string &str, int64_t start, const mixed &length = std::numeric_limits<int64_t>::max(), const string &encoding = CP1251);

void f$set_detect_incorrect_encoding_names_warning(bool show);

void free_detect_incorrect_encoding_names();

#endif

void free_detect_incorrect_encoding_names();
