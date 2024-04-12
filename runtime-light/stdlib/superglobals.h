#pragma once

#include "runtime-light/core/kphp_core.h"
#include "runtime-light/coroutine/task.h"

constexpr int8_t HTTP_MAGIC = 0;
constexpr int8_t COMPONENT_QUERY_MAGIC = 1;

void init_http_superglobals(const char * buffer, int size);

void init_component_superglobals();
