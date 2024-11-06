// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cstdint>

#include "runtime-common/core/runtime-core.h"
#include "runtime-light/component/component.h"
#include "runtime-light/coroutine/task.h"
#include "runtime-light/utils/context.h"

// === Client =====================================================================================

task_t<Optional<int64_t>> f$job_worker_send_request(string request, double timeout) noexcept;

task_t<bool> f$job_worker_send_noreply_request(string request, double timeout) noexcept;

task_t<array<Optional<int64_t>>> f$job_worker_send_multi_request(array<string> requests, double timeout) noexcept;

// === Server =====================================================================================

task_t<string> f$job_worker_fetch_request() noexcept;

task_t<int64_t> f$job_worker_store_response(string response) noexcept;

// === Misc =======================================================================================

inline bool f$is_kphp_job_workers_enabled() noexcept {
  return get_component_context()->image_kind() == ImageKind::Server;
}

inline int64_t f$get_job_workers_number() noexcept {
  return 50; // TODO
}
