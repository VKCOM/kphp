// Compiler for PHP (aka KPHP)
// Copyright (c) 2021 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "runtime/job-workers/job-interface.h"
#include "runtime/kphp_core.h"

struct job_query_data;
void init_job_server_interface_lib(job_query_data job_data) noexcept;
void free_job_server_interface_lib() noexcept;

class_instance<C$KphpJobWorkerRequest> f$kphp_job_worker_fetch_request() noexcept;
int64_t f$kphp_job_worker_store_response(const class_instance<C$KphpJobWorkerResponse> &response) noexcept;
