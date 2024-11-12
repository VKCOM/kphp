// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "runtime-common/core/runtime-core.h"

string f$rawurlencode(const string &s) noexcept;

string f$rawurldecode(const string &s) noexcept;

string f$urlencode(const string &s) noexcept;

string f$urldecode(const string &s) noexcept;
