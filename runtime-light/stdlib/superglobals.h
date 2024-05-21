#pragma once

#include "runtime-core/runtime-core.h"
#include "runtime-light/coroutine/task.h"

enum QueryType {
  HTTP, COMPONENT
};

void init_http_superglobals(const char * buffer, int size);
