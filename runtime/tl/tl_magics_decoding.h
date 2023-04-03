// Compiler for PHP (aka KPHP)
// Copyright (c) 2023 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cstdint>

#include "runtime/kphp_core.h"

const char *tl_function_magic_to_name(uint32_t magic) noexcept;

array<int64_t> tl_magic_get_all_functions() noexcept;

string f$convert_tl_function_magic_to_name(int64_t magic) noexcept;

array<int64_t> f$get_all_tl_function_magics() noexcept;
