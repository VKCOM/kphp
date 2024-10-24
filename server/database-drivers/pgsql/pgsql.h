#pragma once

#include "common/kprintf.h"
#include "runtime-common/core/runtime-core.h"
#include "runtime/allocator.h"
#include "runtime/critical_section.h"

DECLARE_VERBOSITY(pgsql);

#define LIB_PGSQL_CALL(call) (dl::CriticalSectionGuard{}, call)

namespace database_drivers {

void free_pgsql_lib();

} // namespace database_drivers
