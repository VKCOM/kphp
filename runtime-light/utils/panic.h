#pragma once

#include <csetjmp>


#include "runtime-light/context.h"
#include "runtime-light/utils/logs.h"
#include "runtime-light/component/component.h"

inline void panic() {
  constexpr const char * message = "script panic";
  get_platform_context()->log(Info, strlen(message), message);
  siglongjmp(get_component_context()->exit_tag, 1);
}
