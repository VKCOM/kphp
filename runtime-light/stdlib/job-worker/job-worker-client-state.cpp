// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime-light/stdlib/job-worker/job-worker-client-state.h"

#include "runtime-light/state/instance-state.h"

JobWorkerClientInstanceState &JobWorkerClientInstanceState::get() noexcept {
  return InstanceState::get().job_worker_client_instance_state;
}
