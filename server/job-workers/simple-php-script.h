// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

// TODO drop this file and functions
namespace job_workers {

struct SharedMemorySlice;

void store_reply(SharedMemorySlice *reply) noexcept;
SharedMemorySlice *fetch_request() noexcept;

SharedMemorySlice *run_simple_php_script(SharedMemorySlice *request) noexcept;

} // namespace job_workers
