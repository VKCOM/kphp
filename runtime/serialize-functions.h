// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "runtime/kphp_core.h"

string f$serialize(const mixed &v) noexcept;

mixed f$unserialize(const string &v) noexcept;

mixed unserialize_raw(const char *v, int32_t v_len) noexcept;
