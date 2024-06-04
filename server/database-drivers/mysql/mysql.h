// Compiler for PHP (aka KPHP)
// Copyright (c) 2021 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "common/kprintf.h"
#include "runtime/runtime-types.h"
#include "runtime/allocator.h"
#include "runtime/critical_section.h"

DECLARE_VERBOSITY(mysql);

#define LIB_MYSQL_CALL(call) (dl::CriticalSectionGuard{}, call)

namespace database_drivers {

void free_mysql_lib();

} // namespace database_drivers
