#pragma once

#include <utility>
#include <cstdint>

#include "runtime-light/coroutine/task.h"

std::pair<char *, int> read_all_from_stream(uint64_t stream_d);

task_t<bool> write_all_to_stream(uint64_t stream_d, const char * buffer, int len);