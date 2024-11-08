// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "runtime-light/coroutine/awaitable.h"
#include "runtime-light/coroutine/task.h"

// Returns a stream descriptor that is supposed to be a stream to stdout
inline task_t<uint64_t> init_kphp_cli_component() noexcept {
  co_return co_await wait_for_incoming_stream_t{};
}

// Performs some initialization and returns a stream descriptor we need to write server response into
task_t<uint64_t> init_kphp_server_component() noexcept;
