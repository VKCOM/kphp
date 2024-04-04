#pragma once

#include "runtime-light/core/kphp_core.h"
#include "runtime-light/coroutine/task.h"

task_t<void> f$testyield();

void f$check_shutdown();

task_t<void> f$exit(const mixed &v = 0);


task_t<void> f$die(const mixed &v = 0);

void reset();

task_t<void> init();
task_t<void> finish(int64_t exit_code);

