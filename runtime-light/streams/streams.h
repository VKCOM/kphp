#pragma once

#include <utility>
#include <cstdint>

#include "runtime-light/coroutine/task.h"

#include "runtime-light/core/kphp_core.h"

enum StreamRuntimeStatus {
  WBlocked,
  RBlocked,
  NotBlocked
};

task_t<std::pair<char *, int>> read_all_from_stream(uint64_t stream_d);
std::pair<char *, int> read_nonblock_from_stream(uint64_t stream_d);
task_t<int> read_exact_from_stream(uint64_t stream_d, char * buffer, int len);

task_t<int> write_all_to_stream(uint64_t stream_d, const char * buffer, int len);
int write_nonblock_to_stream(uint64_t stream_d, const char * buffer, int len);
task_t<int> write_exact_to_stream(uint64_t stream_d, const char * buffer, int len);

void free_all_descriptors();
void free_descriptor(uint64_t stream_d);
