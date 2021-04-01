// Compiler for PHP (aka KPHP)
// Copyright (c) 2021 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "runtime/job-workers/job-interface.h"
#include "runtime/kphp_core.h"

void free_job_client_interface_lib() noexcept;

int64_t f$kphp_job_worker_start(const class_instance<C$KphpJobWorkerRequest> &request, double timeout = -1.0) noexcept;
class_instance<C$KphpJobWorkerResponse> f$kphp_job_worker_wait(int64_t job_resumable_id, double timeout = -1.0) noexcept;
