//  Compiler for PHP (aka KPHP)
//  Copyright (c) 2024 LLC «V Kontakte»
//  Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cstdint>

#include "runtime-light/core/kphp_core.h"
#include "runtime-light/coroutine/task.h"

constexpr int64_t JOB_WORKER_VALID_JOB_ID_RANGE_START = 0;
constexpr int64_t JOB_WORKER_INVALID_JOB_ID = -1;
constexpr int64_t JOB_WORKER_IGNORED_JOB_ID = -2;

// === Client =====================================================================================

task_t<Optional<int64_t>> f$kphp_job_worker_start(string request, double timeout) noexcept; // TODO: why optional???

task_t<bool> f$kphp_job_worker_start_no_reply(string request, double timeout) noexcept;

task_t<string> f$kphp_job_worker_fetch_response(int64_t job_id) noexcept;

// === Server =====================================================================================

task_t<string> f$kphp_job_worker_fetch_request() noexcept;

task_t<int64_t> f$kphp_job_worker_store_response(string response) noexcept;

// === Misc =======================================================================================
