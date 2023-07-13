// Compiler for PHP (aka KPHP)
// Copyright (c) 2023 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cstdint>

#include "runtime/kphp_core.h"

const char *tl_magic_convert_to_name(uint32_t magic) noexcept;
array<string> tl_magic_get_all_functions() noexcept;
