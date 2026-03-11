// Compiler for PHP (aka KPHP)
// Copyright (c) 2026 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "runtime-light/stdlib/diagnostics/logs.h"

#define SAVE_BUILTIN_CALL_STATS(builtin_name, builtin_call) (kphp::log::debug("built-in called: " builtin_name), builtin_call)
