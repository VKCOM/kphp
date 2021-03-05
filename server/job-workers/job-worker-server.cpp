// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include <cassert>
#include <cstdint>

#include "common/kprintf.h"
#include "common/timer.h"

#include "net/net-events.h"
#include "net/net-reactor.h"

#include "runtime/job-workers/shared-memory-manager.h"
#include "runtime/job-workers/x2-job.h"

#include "server/job-workers/job-worker-server.h"
#include "server/job-workers/job-workers-context.h"
#include "server/job-workers/shared-context.h"
#include "server/job-workers/simple-php-script.h"
#include "server/php-master-restart.h"

namespace job_workers {

int JobWorkerServer::read_jobs(int fd, void *data __attribute__((unused)), event_t *ev) {
  vkprintf(3, "JobWorkerClient::read_jobs: fd=%d\n", fd);
  tvkprintf(job_workers, 3, "wakeup job worker\n");

  auto &job_worker_server = vk::singleton<JobWorkerServer>::get();
  assert(fd == job_worker_server.read_job_fd);

  if (ev->ready & EVT_SPEC) {
    kprintf("job worker server special event: fd = %d, flags = %d\n", fd, ev->ready);
    // TODO:
    return 0;
  }
  if (!(ev->ready & EVT_READ)) {
    kprintf("Strange event in server: fd = %d, ev->ready = 0x%08x\n", fd, ev->ready);
    return 0;
  }

  int status_code = job_worker_server.read_execute_loop();

  return status_code;
}

int JobWorkerServer::read_execute_loop() {
  while (true) {
    if (last_stats.is_started() && last_stats.time() > std::chrono::duration<int>{JobWorkersContext::MAX_HANGING_TIME_SEC / 2}) {
      tvkprintf(job_workers, 1, "Too many jobs. Job worker will complete them later. Goes back to event loop now\n");
      has_delayed_jobs = true;
      return 0;
    }

    Job job;
    PipeJobReader::ReadStatus status = job_reader.read_job(job);

    if (status == PipeJobReader::READ_BLOCK) {
      assert(errno == EWOULDBLOCK);
      // another job worker has already taken the job (all job workers are readers for this fd)
      // or there are no more jobs in pipe
      has_delayed_jobs = false;
      return 0;
    } else if (status == PipeJobReader::READ_FAIL) {
      SharedContext::get().total_errors_pipe_server_read++;
      return -1;
    }

    SharedContext::get().job_queue_size--;

    bool success = execute_job(job);
    if (success) {
      SharedContext::get().total_jobs_done++;
    } else {
      SharedContext::get().total_jobs_failed++;
      kprintf("Error on executing job: <job_result_fd_idx, job_id> = <%d, %d>, job_memory_ptr = %p\n", job.job_result_fd_idx, job.job_id,
              job.job_memory_ptr);
    }
  }
}

bool JobWorkerServer::execute_job(const Job &job) {
  tvkprintf(job_workers, 2, "executing job: <job_result_fd_idx, job_id> = <%d, %d>, job_memory_ptr = %p\n", job.job_result_fd_idx, job.job_id,
            job.job_memory_ptr);

  SharedMemorySlice *reply = run_simple_php_script(job.job_memory_ptr);
  vk::singleton<SharedMemoryManager>::get().release_slice(job.job_memory_ptr);
  int write_job_result_fd = vk::singleton<JobWorkersContext>::get().result_pipes.at(job.job_result_fd_idx)[1];
  bool success = job_writer.write_job_result(JobResult{0, job.job_id, reply}, write_job_result_fd);
  if (!success) {
    vk::singleton<SharedMemoryManager>::get().release_slice(reply);
    SharedContext::get().total_errors_pipe_server_write++;
    return false;
  }

  return true;
}

void JobWorkerServer::init() {
  const auto &job_workers_ctx = vk::singleton<JobWorkersContext>::get();

  assert(job_workers_ctx.pipes_inited);

  read_job_fd = job_workers_ctx.job_pipe[0];
  close(job_workers_ctx.job_pipe[1]); // this endpoint is for HTTP worker to write job
  clear_event(job_workers_ctx.job_pipe[1]);
  for (auto &result_pipe : job_workers_ctx.result_pipes) {
    close(result_pipe[0]); // this endpoint is for HTTP worker to read job result
    clear_event(result_pipe[0]);
  }

  epoll_sethandler(read_job_fd, 0, read_jobs, nullptr);
  epoll_insert(read_job_fd, EVT_READ | EVT_SPEC);
  tvkprintf(job_workers, 1, "insert read_job_fd = %d to epoll\n", read_job_fd);

  job_reader = PipeJobReader{read_job_fd};
}

void JobWorkerServer::try_complete_delayed_jobs() {
  if (has_delayed_jobs) {
    tvkprintf(job_workers, 1, "There are delayed jobs. Let's complete them\n");
    read_execute_loop();
  }
}

} // namespace job_workers
