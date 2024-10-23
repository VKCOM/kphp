// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime-light/stdlib/job-worker/job-worker-client-context.h"

#include "runtime-light/component/component.h"
#include "runtime-light/utils/context.h"

JobWorkerClientComponentContext &JobWorkerClientComponentContext::get() noexcept {
  return get_component_context()->job_worker_client_component_context;
}
