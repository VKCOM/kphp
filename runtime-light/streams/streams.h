// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cstdint>
#include <utility>

#include "runtime-light/coroutine/task.h"

/**
 * **Oneshot component only**
 * Wait for initial stream and process it. There can be 2 types of initial (or starter) streams: 1. http; 2. job worker.
 */
task_t<uint64_t> accept_initial_stream() noexcept;

// === read =======================================================================================

task_t<std::pair<char *, int32_t>> read_all_from_stream(uint64_t stream_d) noexcept;

std::pair<char *, int32_t> read_nonblock_from_stream(uint64_t stream_d) noexcept;

task_t<int32_t> read_exact_from_stream(uint64_t stream_d, char *buffer, int32_t len) noexcept;

// === write ======================================================================================

task_t<int32_t> write_all_to_stream(uint64_t stream_d, const char *buffer, int32_t len) noexcept;

int32_t write_nonblock_to_stream(uint64_t stream_d, const char *buffer, int32_t len) noexcept;

task_t<int32_t> write_exact_to_stream(uint64_t stream_d, const char *buffer, int32_t len) noexcept;
