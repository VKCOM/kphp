// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cstdint>
#include <utility>

#include "runtime-light/coroutine/task.h"

// === read =======================================================================================

task_t<std::pair<char *, int32_t>> read_all_from_stream(uint64_t stream_d);

std::pair<char *, int32_t> read_nonblock_from_stream(uint64_t stream_d);

task_t<int32_t> read_exact_from_stream(uint64_t stream_d, char *buffer, int32_t len);

// === write ======================================================================================

task_t<int32_t> write_all_to_stream(uint64_t stream_d, const char *buffer, int32_t len);

int32_t write_nonblock_to_stream(uint64_t stream_d, const char *buffer, int32_t len);

task_t<int32_t> write_exact_to_stream(uint64_t stream_d, const char *buffer, int32_t len);
