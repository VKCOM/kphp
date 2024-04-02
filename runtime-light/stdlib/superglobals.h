#pragma once

#include "runtime-light/core/kphp_core.h"
#include "runtime-light/coroutine/task.h"

string v$_POST    __attribute__ ((weak));

task_t<void> init_superglobals();
