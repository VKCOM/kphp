#pragma once

#include "runtime-light/core/kphp_core.h"
#include "runtime-light/coroutine/task.h"

int64_t f$rand();

task_t<string> f$query_echo(const string & str);

task_t<void> f$dummy_echo();
