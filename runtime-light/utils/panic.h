#pragma once

#include <setjmp.h>

#include "runtime-light/context.h"

inline void panic() {
  constexpr const char * message = "script OOM\n";
  get_platform_context()->log(2, strlen(message), message);
  siglongjmp(get_component_context()->exit_tag, 1);
}
