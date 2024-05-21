#pragma once

#include "runtime-core/runtime-core.h"
#include "runtime-light/coroutine/task.h"

int64_t f$rand();

template<class T>
T f$make_clone(const T &x) {
  return x;
}
