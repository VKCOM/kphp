// Compiler for PHP (aka KPHP)
// Copyright (c) 2021 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "runtime/runtime-types.h"
#include "runtime/job-workers/job-interface.h"

void free_job_client_interface_lib() noexcept;

Optional<int64_t> f$kphp_job_worker_start(const class_instance<C$KphpJobWorkerRequest> &request, double timeout) noexcept;
bool f$kphp_job_worker_start_no_reply(const class_instance<C$KphpJobWorkerRequest> &request, double timeout) noexcept;
array<Optional<int64_t>> f$kphp_job_worker_start_multi(const array<class_instance<C$KphpJobWorkerRequest>> &requests, double timeout) noexcept;
