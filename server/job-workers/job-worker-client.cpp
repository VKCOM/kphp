// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include <cassert>
#include <unistd.h>

#include "common/kprintf.h"

#include "net/net-events.h"
#include "net/net-reactor.h"

#include "runtime/job-workers/shared-memory-manager.h"

#include "server/job-workers/job-worker-client.h"
#include "server/job-workers/job-workers-context.h"
#include "server/job-workers/shared-context.h"
#include "server/php-queries.h"

namespace job_workers {

int JobWorkerClient::read_job_results(int fd, void *data __attribute__((unused)), event_t *ev) {
  vkprintf(3, "JobWorkerClient::read_job_results: fd=%d\n", fd);

  auto &job_worker_client = vk::singleton<JobWorkerClient>::get();
  assert(fd == job_worker_client.read_job_result_fd);
  if (ev->ready & EVT_SPEC) {
    kprintf("job worker client special event: fd = %d, flags = %d\n", fd, ev->ready);
    // TODO:
    return 0;
  }
  if (!(ev->ready & EVT_READ)) {
    kprintf("Strange event in client: fd = %d, ev->ready = 0x%08x\n", fd, ev->ready);
    return 0;
  }

  PipeJobReader::ReadStatus status{};

  do {
    JobResult job_result;
    status = job_worker_client.job_reader.read_job_result(job_result);
    if (status == PipeJobReader::READ_FAIL) {
      SharedContext::get().total_errors_pipe_client_read++;
      return -1;
    }
    if (status == PipeJobReader::READ_OK) {
      tvkprintf(job_workers, 2, "got job result: ready_job_id = %d, job_result_memory_ptr = %p\n", job_result.job_id, job_result.job_result_memory_ptr);
      if (create_job_worker_answer_event(job_result.job_id, job_result.job_result_memory_ptr) <= 0) {
        // TODO ERROR?
        vk::singleton<SharedMemoryManager>::get().release_slice(job_result.job_result_memory_ptr);
      }
    }
  } while (status != PipeJobReader::READ_BLOCK);

  return 0;
}

void JobWorkerClient::init(int job_result_slot) {
  auto &job_workers_ctx = vk::singleton<JobWorkersContext>::get();

  assert(job_workers_ctx.pipes_inited);

  write_job_fd = job_workers_ctx.job_pipe[1];
  close(job_workers_ctx.job_pipe[0]); // this endpoint is for job worker to read job
  clear_event(job_workers_ctx.job_pipe[0]);

  for (int i = 0; i < job_workers_ctx.result_pipes.size(); ++i) {
    auto &result_pipe = job_workers_ctx.result_pipes[i];
    if (i == job_result_slot) {
      read_job_result_fd = result_pipe[0];
      job_result_fd_idx = job_result_slot;
    } else {
      close(result_pipe[0]); // this endpoint is for another HTTP worker to read job result
      clear_event(result_pipe[0]);
    }
    close(result_pipe[1]); // this endpoint is for job worker to write job result
    clear_event(result_pipe[1]);
  }

  if (read_job_result_fd >= 0) {
    epoll_sethandler(read_job_result_fd, 0, JobWorkerClient::read_job_results, nullptr);
    epoll_insert(read_job_result_fd, EVT_READ | EVT_SPEC);
    job_reader = PipeJobReader{read_job_result_fd};
  }
}

int JobWorkerClient::send_job(SharedMemorySlice * const job_memory_ptr) {
  static_assert(sizeof(job_memory_ptr) == 8, "Unexpected pointer size");

  slot_id_t job_id = parallel_job_ids_factory.create_slot();

  tvkprintf(job_workers, 2, "sending job: <job_result_fd_idx, job_id> = <%d, %d> , job_memory_ptr = %p, write_job_fd = %d\n", job_result_fd_idx, job_id,
            job_memory_ptr, write_job_fd);

  bool success = job_writer.write_job(Job{job_id, job_result_fd_idx, job_memory_ptr}, write_job_fd);
  if (!success) {
    SharedContext::get().total_errors_pipe_client_write++;
    return -1;
  }

  SharedContext::get().job_queue_size++;
  SharedContext::get().total_jobs_sent++;
  return job_id;
}

} // namespace job_workers
