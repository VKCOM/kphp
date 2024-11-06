// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime-light/server/job-worker/job-worker-server-state.h"

#include "runtime-light/state/instance-state.h"

JobWorkerServerInstanceState &JobWorkerServerInstanceState::get() noexcept {
  return InstanceState::get().job_worker_server_instance_state;
}
