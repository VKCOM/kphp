// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cstdint>

#include "runtime-common/core/runtime-core.h"

Optional<string> f$inet_pton(const string& address) noexcept;

Optional<int64_t> f$ip2long(const string& ip) noexcept;

Optional<string> f$ip2ulong(const string& ip) noexcept;

string f$long2ip(int64_t num) noexcept;
