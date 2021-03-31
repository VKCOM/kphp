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

#include "runtime/job-workers/job-message.h"
#include "runtime/job-workers/shared-memory-manager.h"

#include "server/job-workers/job-worker-server.h"
#include "server/job-workers/job-workers-context.h"
#include "server/job-workers/job-stats.h"
#include "server/php-engine-vars.h"
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
  vk::singleton<JobWorkerServer>::get().reset_running_job();
  assert (c->status != conn_wait_net);
}

int jobs_server_php_wakeup(connection *c) {
  assert (c->status == conn_expect_query || c->status == conn_wait_net);
  c->status = conn_expect_query;

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

  if (running_job) {
    tvkprintf(job_workers, 1, "Get new job while another job is running. Goes back to event loop now\n");
    has_delayed_jobs = true;
    JobStats::get().job_worker_skip_job_due_another_is_running++;
    return 0;
  }

  if (last_stats.is_started() && last_stats.time() > std::chrono::duration<int>{JobWorkersContext::MAX_HANGING_TIME_SEC / 2}) {
    tvkprintf(job_workers, 1, "Too many jobs. Job worker will complete them later. Goes back to event loop now\n");
    has_delayed_jobs = true;
    JobStats::get().job_worker_skip_job_due_overload++;
    return 0;
  }

  JobSharedMessage *job = nullptr;
  PipeJobReader::ReadStatus status = job_reader.read_job(job);

  if (status == PipeJobReader::READ_BLOCK) {
    assert(errno == EWOULDBLOCK);
    // another job worker has already taken the job (all job workers are readers for this fd)
    // or there are no more jobs in pipe
    has_delayed_jobs = false;
    JobStats::get().job_worker_skip_job_due_steal++;
    return 0;
  } else if (status == PipeJobReader::READ_FAIL) {
    JobStats::get().errors_pipe_server_read++;
    return -1;
  }

  JobStats::get().job_queue_size--;
  running_job = job;
  reply_was_sent = false;

  double timeout_sec = job->script_execution_timeout < 0 ? MAX_SCRIPT_TIMEOUT : job->script_execution_timeout;
  job_query_data *job_data = job_query_data_create(job, [](JobSharedMessage *job_response) {
    return vk::singleton<JobWorkerServer>::get().send_job_reply(job_response);
  });
  reinterpret_cast<JobCustomData *>(c->custom_data)->worker = php_worker_create(job_worker, c, nullptr, nullptr, job_data, job->job_id, timeout_sec);

  set_connection_timeout(c, timeout_sec);
  c->status = conn_wait_net;
  jobs_server_php_wakeup(c);

  return 0;
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
  if (!running_job && has_delayed_jobs) {
    tvkprintf(job_workers, 1, "There are delayed jobs. Let's complete them\n");
    job_parse_execute(read_job_connection);
  }
}

void JobWorkerServer::reset_running_job() noexcept {
  running_job = nullptr;
}

const char *JobWorkerServer::send_job_reply(JobSharedMessage *job_response) noexcept {
  if (!running_job) {
    return "There is no running jobs";
  }

  if (reply_was_sent) {
    return "The reply has been already sent";
  }

  int write_job_result_fd = vk::singleton<JobWorkersContext>::get().result_pipes.at(running_job->job_result_fd_idx)[1];
  job_response->job_id = running_job->job_id;
  if (!job_writer.write_job_result(job_response, write_job_result_fd)) {
    JobStats::get().errors_pipe_server_write++;
    return "Can't write job reply to the pipe";
  }
  JobStats::get().jobs_replied++;
  reply_was_sent = true;
  return nullptr;
}

void JobWorkerServer::try_store_job_response_error(const char *error_msg, int error_code) {
  php_assert(running_job);
  if (reply_was_sent) {
    return;
  }
  auto *response_memory = vk::singleton<job_workers::SharedMemoryManager>::get().acquire_shared_message();
  if (!response_memory) {
    kprintf("Can't store job response error: not enough shared memory");
    return;
  }

  dl::set_current_script_allocator(response_memory->resource, false);

  class_instance<C$KphpJobWorkerResponseError> error;
  error.alloc();  // TODO: handle OOM
  error.get()->error = string{error_msg};
  error.get()->error_code = error_code;
  response_memory->instance = std::move(error);

  dl::restore_default_script_allocator(false);

  if (const char *err = send_job_reply(response_memory)) {
    vk::singleton<job_workers::SharedMemoryManager>::get().release_shared_message(response_memory);
    kprintf("Can't store job response: %s", err);
  }
}

} // namespace job_workers
