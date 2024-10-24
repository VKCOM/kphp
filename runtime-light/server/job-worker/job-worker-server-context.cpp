// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "runtime-light/server/job-worker/job-worker-server-context.h"

#include "runtime-light/component/component.h"
#include "runtime-light/utils/context.h"

JobWorkerServerComponentContext &JobWorkerServerComponentContext::get() noexcept {
  return get_component_context()->job_worker_server_component_context;
}
