#pragma once

#include "runtime-light/core/kphp_core.h"
#include "runtime-light/coroutine/task.h"

struct Superglobals {
  string v$POST;
};


task_t<void> init_superglobals();
