// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <climits>

namespace job_workers {

struct Job {
  int job_id{};
  int job_result_fd_idx{};
  void *job_memory_ptr{};

  Job() = default;

  Job(int job_id, int job_result_fd_idx, void *job_memory_ptr)
    : job_id(job_id)
    , job_result_fd_idx(job_result_fd_idx)
    , job_memory_ptr(job_memory_ptr) {}
};

static_assert(PIPE_BUF % sizeof(Job) == 0, "Size of job must be multiple of PIPE_BUF for read/write atomicity");

struct JobResult {
  int padding{};
  int job_id{};
  void *job_result_memory_ptr{};

  JobResult() = default;

  JobResult(int padding, int job_id, void *job_result_memory_ptr)
    : padding(padding)
    , job_id(job_id)
    , job_result_memory_ptr(job_result_memory_ptr) {}
};

static_assert(PIPE_BUF % sizeof(JobResult) == 0, "Size of job result must be multiple of PIPE_BUF for read/write atomicity");

} // namespace job_workers
