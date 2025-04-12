// Compiler for PHP (aka KPHP)
// Copyright (c) 2022 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "runtime-common/core/runtime-core.h"

bool f$ctype_alnum(const mixed &text) noexcept;
bool f$ctype_alpha(const mixed &text) noexcept;
bool f$ctype_cntrl(const mixed &text) noexcept;
bool f$ctype_digit(const mixed &text) noexcept;
bool f$ctype_graph(const mixed &text) noexcept;
bool f$ctype_lower(const mixed &text) noexcept;
bool f$ctype_print(const mixed &text) noexcept;
bool f$ctype_punct(const mixed &text) noexcept;
bool f$ctype_space(const mixed &text) noexcept;
bool f$ctype_upper(const mixed &text) noexcept;
bool f$ctype_xdigit(const mixed &text) noexcept;
