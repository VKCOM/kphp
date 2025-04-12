// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cstdint>

#include "runtime-common/core/runtime-core.h"
#include "runtime-common/stdlib/string/string-context.h"

bool mb_UTF8_check(const char* s) noexcept;

bool f$mb_check_encoding(const string& str, const string& encoding = StringLibConstants::get().CP1251_STR) noexcept;

int64_t f$mb_strlen(const string& str, const string& encoding = StringLibConstants::get().CP1251_STR) noexcept;

string f$mb_strtolower(const string& str, const string& encoding = StringLibConstants::get().CP1251_STR) noexcept;

string f$mb_strtoupper(const string& str, const string& encoding = StringLibConstants::get().CP1251_STR) noexcept;

Optional<int64_t> f$mb_strpos(const string& haystack, const string& needle, int64_t offset = 0,
                              const string& encoding = StringLibConstants::get().CP1251_STR) noexcept;

Optional<int64_t> f$mb_stripos(const string& haystack, const string& needle, int64_t offset = 0,
                               const string& encoding = StringLibConstants::get().CP1251_STR) noexcept;

string f$mb_substr(const string& str, int64_t start, const mixed& length = std::numeric_limits<int64_t>::max(),
                   const string& encoding = StringLibConstants::get().CP1251_STR) noexcept;
