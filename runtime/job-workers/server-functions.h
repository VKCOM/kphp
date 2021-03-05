// Compiler for PHP (aka KPHP)
// Copyright (c) 2021 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "runtime/job-workers/job-interface.h"
#include "runtime/kphp_core.h"

void init_job_server_interface_lib(job_workers::SharedMemorySlice *request_memory, const char *(*send_reply) (job_workers::SharedMemorySlice *)) noexcept;
void free_job_server_interface_lib() noexcept;

class_instance<C$KphpJobWorkerRequest> f$job_worker_fetch_request() noexcept;
void f$job_worker_store_response(const class_instance<C$KphpJobWorkerReply> &response) noexcept;
