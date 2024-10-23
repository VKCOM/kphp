// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <climits>

#include "runtime-common/runtime-core/runtime-core.h"
#include "runtime/string_functions.h"

bool mb_UTF8_check(const char *s);

bool f$mb_check_encoding(const string &str, const string &encoding = CP1251);

int64_t f$mb_strlen(const string &str, const string &encoding = CP1251);

string f$mb_strtolower(const string &str, const string &encoding = CP1251);

string f$mb_strtoupper(const string &str, const string &encoding = CP1251);

Optional<int64_t> f$mb_strpos(const string &haystack, const string &needle, int64_t offset = 0, const string &encoding = CP1251) noexcept;

Optional<int64_t> f$mb_stripos(const string &haystack, const string &needle, int64_t offset = 0, const string &encoding = CP1251) noexcept;

string f$mb_substr(const string &str, int64_t start, const mixed &length = std::numeric_limits<int64_t>::max(), const string &encoding = CP1251);

void f$set_detect_incorrect_encoding_names_warning(bool show);

void free_detect_incorrect_encoding_names();
