#pragma once

#include "runtime-light/core/kphp_core.h"
#include "runtime-light/coroutine/task.h"

int64_t f$open(const string & name);

task_t<void> f$send(int64_t id, const string & message);

task_t<string> f$read(int64_t id);

void f$close(int64_t id);
