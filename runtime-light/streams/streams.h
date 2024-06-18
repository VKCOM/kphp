// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cstdint>
#include <utility>

#include "runtime-core/runtime-core.h"

#include "runtime-light/coroutine/task.h"

enum class DescriptorRuntimeStatus {
  Stream,
  Timer
};

constexpr uint64_t BAD_PLATFORM_DESCRIPTOR = 0;

void register_stream(int64_t stream_d);
void register_timer(int64_t timer_d);

task_t<std::pair<char *, int>> read_all_from_stream(uint64_t stream_d);
std::pair<char *, int> read_nonblock_from_stream(uint64_t stream_d);
task_t<int> read_exact_from_stream(uint64_t stream_d, char *buffer, int len);

task_t<int> write_all_to_stream(uint64_t stream_d, const char *buffer, int len);
int write_nonblock_to_stream(uint64_t stream_d, const char *buffer, int len);
task_t<int> write_exact_to_stream(uint64_t stream_d, const char *buffer, int len);

void free_all_descriptors();
void free_descriptor(uint64_t stream_d);
