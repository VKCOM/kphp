// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "runtime-common/core/runtime-core.h"
#include "runtime-light/coroutine/task.h"

// Returns a stream descriptor that is supposed to be a stream to stdout
task_t<uint64_t> init_kphp_cli_component() noexcept;

task_t<void> finalize_kphp_cli_component(const string_buffer &output) noexcept;

// Performs some initialization and returns a stream descriptor we need to write server response into
task_t<uint64_t> init_kphp_server_component() noexcept;

task_t<void> finalize_kphp_server_component(const string_buffer &output) noexcept;
