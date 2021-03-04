// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include <cassert>
#include <cstdint>

#include "common/kprintf.h"
#include "common/pipe-utils.h"
#include "common/timer.h"

#include "net/net-events.h"
#include "net/net-connections.h"

#include "server/job-workers/job-worker-server.h"
#include "server/job-workers/job-workers-context.h"
#include "server/job-workers/shared-context.h"
#include "server/job-workers/shared-memory-manager.h"
#include "server/php-master-restart.h"
#include "server/php-worker.h"

namespace job_workers {

namespace {

struct JobCustomData {
  php_worker *worker;
};

int jobs_server_reader(connection *c) {
  int status = c->type->parse_execute(c);

  c->flags |= C_NORD;
  c->flags &= ~C_REPARSE;

  if (status < 0) {
    c->error = -1;
    status = -1;
  }

  return status;
}

int jobs_server_parse_execute(connection *c) {
  return vk::singleton<JobWorkerServer>::get().job_parse_execute(c);
}

void jobs_server_at_query_end(connection *c) {
  clear_connection_timeout(c);
  c->generation = ++conn_generation;
  c->pending_queries = 0;

  c->flags |= C_REPARSE;

  vk::singleton<JobWorkerServer>::get().running_job.reset();
  assert (c->status != conn_wait_net);
}

int jobs_server_php_wakeup(connection *c) {
  assert (c->status == conn_expect_query || c->status == conn_wait_net);
  c->status = conn_expect_query;

//  JobWorkerServer &job_worker_server = vk::singleton<JobWorkerServer>::get();
//  Job &job = job_worker_server.running_job.value();
//  bool success = job_worker_server.execute_job(job);
//  if (success) {
//    SharedContext::get().total_jobs_done++;
//  } else {
//    SharedContext::get().total_jobs_failed++;
//    kprintf("Error on executing job: <job_result_fd_idx, job_id> = <%d, %d>, job_memory_ptr = %p\n", job.job_result_fd_idx, job.job_id,
//            job.job_memory_ptr);
//  }

  auto *worker = reinterpret_cast<JobCustomData *>(c->custom_data)->worker;
  double timeout = php_worker_main(worker);

  if (timeout == 0) {
    jobs_server_at_query_end(c);
  } else {
    assert (c->pending_queries >= 0 && c->status == conn_wait_net);
    assert (timeout > 0);
    set_connection_timeout(c, timeout);
  }
  return 0;
}


conn_type_t php_jobs_server = [] {
  auto res = conn_type_t();

  res.flags = 0;
  res.magic = CONN_FUNC_MAGIC;
  res.title = "jobs_server";

  res.run = server_read_write;
  res.reader = jobs_server_reader;
  res.parse_execute = jobs_server_parse_execute;
  res.wakeup = jobs_server_php_wakeup;
  res.alarm = jobs_server_php_wakeup;

  res.accept = server_failed;
  res.init_accepted = server_failed;
  res.writer = server_failed;
  res.init_outbound = server_failed;
  res.connected = server_failed;
  res.close = server_failed;
  res.free_buffers = server_failed;

  // The following handlers will be set to default ones:
  //    flush
  //    check_ready

  return res;
}();

} // namespace

int JobWorkerServer::job_parse_execute(connection *c) {
  assert(c == read_job_connection);

  if (running_job.has_value()) {
    tvkprintf(job_workers, 1, "Get new job while another job is running. Goes back to event loop now\n");
    has_delayed_jobs = true;
    return 0;
  }

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
  running_job = job;

  double timeout_sec = DEFAULT_SCRIPT_TIMEOUT;
  job_query_data *job_data = job_query_data_create(job);
  reinterpret_cast<JobCustomData *>(c->custom_data)->worker = php_worker_create(job_worker, c, nullptr, nullptr, job_data, job.job_id, timeout_sec);

  set_connection_timeout(c, timeout_sec);
  c->status = conn_wait_net;
  jobs_server_php_wakeup(c);

  return 0;
}

static bool run_job_array_x2(int64_t *dest, const int64_t *src, int64_t size) {
  static constexpr auto HANG_SHIFT = static_cast<int64_t>(1e9);
  for (int64_t i = 0; i < size; ++i) {
    int64_t item = src[i];
    if (item < 0) {
      return false;
    } else if (item > HANG_SHIFT) {
      int64_t hang_time_ms = item - HANG_SHIFT;
      vk::SteadyTimer<std::chrono::milliseconds> timer;
      timer.start();
      while (timer.time() < std::chrono::milliseconds{hang_time_ms}) {
      };
    }
    dest[i] = item * item;
  }
  return true;
}

bool JobWorkerServer::execute_job(const Job &job) {
  tvkprintf(job_workers, 2, "executing job: <job_result_fd_idx, job_id> = <%d, %d>, job_memory_ptr = %p\n", job.job_result_fd_idx, job.job_id,
            job.job_memory_ptr);

  SharedMemoryManager &memory_manager = vk::singleton<SharedMemoryManager>::get();

  void * const job_result_memory_slice = memory_manager.allocate_slice(); // TODO: reuse job_memory_ptr ?
  if (job_result_memory_slice == nullptr) {
    kprintf("Can't allocate slice for result of job: not enough shared memory\n");
    SharedContext::get().total_errors_shared_memory_limit++;
    memory_manager.deallocate_slice(job.job_memory_ptr);
    return false;
  }

  auto *job_memory = static_cast<int64_t *>(job.job_memory_ptr);
  int64_t arr_size = *job_memory++;

  auto *job_result_memory = reinterpret_cast<int64_t *>(job_result_memory_slice);
  *job_result_memory++ = arr_size;

  bool ok = run_job_array_x2(job_result_memory, job_memory, arr_size);
  memory_manager.deallocate_slice(job.job_memory_ptr);

  if (!ok) {
    memory_manager.deallocate_slice(job_result_memory_slice);
    assert(false);
  }

  int write_job_result_fd = vk::singleton<JobWorkersContext>::get().result_pipes.at(job.job_result_fd_idx)[1];

  bool success = job_writer.write_job_result(JobResult{0, job.job_id, job_result_memory_slice}, write_job_result_fd);
  if (!success) {
    memory_manager.deallocate_slice(job_result_memory_slice);
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

  read_job_connection = epoll_insert_pipe(pipe_for_read, read_job_fd, &php_jobs_server, nullptr);
  assert(read_job_connection);
  memset(read_job_connection->custom_data, 0, sizeof(read_job_connection->custom_data));

  tvkprintf(job_workers, 1, "insert read job connection [fd = %d] to epoll\n", read_job_connection->fd);

  job_reader = PipeJobReader{read_job_fd};
}

void JobWorkerServer::try_complete_delayed_jobs() {
  if (!running_job.has_value() && has_delayed_jobs) {
    tvkprintf(job_workers, 1, "There are delayed jobs. Let's complete them\n");
    job_parse_execute(read_job_connection);
  }
}

} // namespace job_workers
