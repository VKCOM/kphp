// Compiler for PHP (aka KPHP)
// Copyright (c) 2021 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "runtime/kphp_core.h"

namespace job_workers {

struct SharedMemorySlice;

// server function
SharedMemorySlice *process_x2(SharedMemorySlice *req) noexcept;

} // namespace job_workers

// client function
int64_t f$async_x2(const array<int64_t> &arr) noexcept;
array<int64_t> f$await_x2(int64_t job_id) noexcept;
