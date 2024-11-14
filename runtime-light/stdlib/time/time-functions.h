// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cstdint>

#include "runtime-common/core/runtime-core.h"

int64_t f$_hrtime_int() noexcept;

string f$_microtime_string() noexcept;
double f$_microtime_float() noexcept;

int64_t f$time() noexcept;

int64_t f$mktime(int64_t hour, Optional<int64_t> minute = {}, Optional<int64_t> second = {}, Optional<int64_t> month = {}, Optional<int64_t> day = {},
                 Optional<int64_t> year = {}) noexcept;

string f$gmdate(const string &format, Optional<int64_t> timestamp = {}) noexcept;

string f$date(const string &format, Optional<int64_t> timestamp = {}) noexcept;

bool f$date_default_timezone_set(const string &s) noexcept;
