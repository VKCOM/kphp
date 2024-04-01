#pragma once

#include "runtime-light/core/kphp_core.h"
#include "runtime-light/coroutine/task.h"

task_t<void> f$yield();

void f$check_shutdown();

task_t<void> f$exit(const mixed &v = 0);


task_t<void> f$die(const mixed &v = 0);


task_t<void> finish(int64_t exit_code);

