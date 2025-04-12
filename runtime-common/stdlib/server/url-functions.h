// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "runtime-common/core/runtime-core.h"

string f$base64_encode(const string &s) noexcept;

Optional<string> f$base64_decode(const string &s, bool strict = false) noexcept;

string f$rawurlencode(const string &s) noexcept;

string f$rawurldecode(const string &s) noexcept;

string f$urlencode(const string &s) noexcept;

string f$urldecode(const string &s) noexcept;

mixed f$parse_url(const string &s, int64_t component = -1) noexcept;

void parse_str_set_value(mixed &arr, const string &key, const string &value) noexcept;

void f$parse_str(const string &str, mixed &arr) noexcept;
