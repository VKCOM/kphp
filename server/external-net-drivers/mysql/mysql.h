// Compiler for PHP (aka KPHP)
// Copyright (c) 2021 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "common/kprintf.h"
#include "runtime/allocator.h"

DECLARE_VERBOSITY(mysql);

#define LIB_MYSQL_CALL(call) (dl::MallocReplacementGuard{}, call)
