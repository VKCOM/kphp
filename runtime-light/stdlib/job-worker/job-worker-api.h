// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cstdint>

#include "runtime-common/core/runtime-core.h"
#include "runtime-light/coroutine/task.h"

// === Client =====================================================================================

kphp::coro::task<Optional<int64_t>> f$job_worker_send_request(string request, double timeout) noexcept;

kphp::coro::task<bool> f$job_worker_send_noreply_request(string request, double timeout) noexcept;

kphp::coro::task<array<Optional<int64_t>>> f$job_worker_send_multi_request(array<string> requests, double timeout) noexcept;

// === Server =====================================================================================

kphp::coro::task<string> f$job_worker_fetch_request() noexcept;

kphp::coro::task<int64_t> f$job_worker_store_response(string response) noexcept;

// === Misc =======================================================================================

inline bool f$is_kphp_job_workers_enabled() noexcept {
  return false;
  // return InstanceState::get().image_kind() == image_kind::server;
}

inline int64_t f$get_job_workers_number() noexcept {
  return 50; // TODO
}
