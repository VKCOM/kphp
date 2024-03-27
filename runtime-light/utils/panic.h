#pragma once

#include <setjmp.h>

#include "runtime-light/context.h"

inline void panic() {
  siglongjmp(get_component_context()->panic_buffer, 1);
}
