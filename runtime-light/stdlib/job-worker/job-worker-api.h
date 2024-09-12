// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cstdint>

#include "runtime-core/runtime-core.h"
#include "runtime-light/component/component.h"
#include "runtime-light/coroutine/task.h"
#include "runtime-light/utils/context.h"

// === Client =====================================================================================

task_t<Optional<int64_t>> f$kphp_job_worker_start(string request, double timeout) noexcept;

task_t<bool> f$kphp_job_worker_start_no_reply(string request, double timeout) noexcept;

task_t<array<Optional<int64_t>>> f$kphp_job_worker_start_multi(array<string> requests, double timeout) noexcept;

// === Server =====================================================================================

task_t<string> f$kphp_job_worker_fetch_request() noexcept;

task_t<int64_t> f$kphp_job_worker_store_response(string response) noexcept;

// === Misc =======================================================================================

inline bool f$is_kphp_job_workers_enabled() noexcept {
  return get_component_context()->component_kind() == ComponentKind::Server;
}

inline int64_t f$get_job_workers_number() noexcept {
  return 50; // TODO
}
