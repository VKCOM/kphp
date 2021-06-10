// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include <cassert>
#include <chrono>

#include "common/kprintf.h"
#include "common/pipe-utils.h"

#include "net/net-events.h"
#include "net/net-connections.h"

#include "server/job-workers/job-message.h"
#include "server/job-workers/job-worker-server.h"
#include "server/job-workers/job-workers-context.h"
#include "server/job-workers/job-stats.h"
#include "server/job-workers/shared-memory-manager.h"
#include "server/php-master-restart.h"
#include "server/php-worker.h"
#include "server/php-worker-stats.h"
#include "server/server-log.h"

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
  res.free_buffers = server_failed;
  res.close = server_noop;    // can be on master termination, when fd is closed before SIGKILL is received

  // The following handlers will be set to default ones:
  //    flush
  //    check_ready

  return res;
}();

} // namespace

int JobWorkerServer::job_parse_execute(connection *c) {
  assert(c == read_job_connection);

  if (sigterm_on) {
    tvkprintf(job_workers, 1, "Get new job after SIGTERM. Ignore it\n");
    return 0;
  }

  if (running_job) {
    tvkprintf(job_workers, 3, "Get new job while another job is running. Goes back to event loop now\n");
    ++vk::singleton<SharedMemoryManager>::get().get_stats().job_worker_skip_job_due_another_is_running;
    return 0;
  }

  if (last_stats.is_started() && last_stats.time() > std::chrono::duration<int>{JobWorkersContext::MAX_HANGING_TIME_SEC / 2}) {
    tvkprintf(job_workers, 1, "Too many jobs. Job worker will complete them later. Goes back to event loop now\n");
    ++vk::singleton<SharedMemoryManager>::get().get_stats().job_worker_skip_job_due_overload;
    return 0;
  }

  JobSharedMessage *job = nullptr;
  PipeJobReader::ReadStatus status = job_reader.read_job(job);

  if (status == PipeJobReader::READ_BLOCK) {
    assert(errno == EWOULDBLOCK);
    // another job worker has already taken the job (all job workers are readers for this fd)
    // or there are no more jobs in pipe
    ++vk::singleton<SharedMemoryManager>::get().get_stats().job_worker_skip_job_due_steal;
    return 0;
  } else if (status == PipeJobReader::READ_FAIL) {
    ++vk::singleton<SharedMemoryManager>::get().get_stats().errors_pipe_server_read;
    return -1;
  }

  auto &memory_manager = vk::singleton<job_workers::SharedMemoryManager>::get();
  --memory_manager.get_stats().job_queue_size;
  memory_manager.attach_shared_message_to_this_proc(job);
  if (job->parent_job) {
    memory_manager.attach_shared_message_to_this_proc(job->parent_job);
  }
  running_job = job;
  reply_was_sent = false;

  auto now = std::chrono::system_clock::now();
  double now_time = std::chrono::duration<double>{now.time_since_epoch()}.count();

  double job_wait_time = now_time - job->job_start_time;
  double left_job_time = job->job_deadline_time() - now_time;

  tvkprintf(job_workers, 2, "got new job: <job_result_fd_idx, job_id> = <%d, %d> , job_memory_ptr = %p, left_job_time = %f, job_wait_time = %f\n",
            job->job_result_fd_idx, job->job_id, job, left_job_time, job_wait_time);

  PhpWorkerStats::get_local().add_job_wait_time_stats(job_wait_time);

  job_query_data *job_data = job_query_data_create(job, [](JobSharedMessage *job_response) {
    return vk::singleton<JobWorkerServer>::get().send_job_reply(job_response);
  });
  reinterpret_cast<JobCustomData *>(c->custom_data)->worker = php_worker_create(job_worker, c, nullptr, nullptr, job_data, job->job_id, left_job_time);

  set_connection_timeout(c, left_job_time);
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

  read_job_connection = epoll_insert_pipe(pipe_for_read, read_job_fd, &php_jobs_server, nullptr, EVT_LEVEL);
  assert(read_job_connection);
  memset(read_job_connection->custom_data, 0, sizeof(read_job_connection->custom_data));

  tvkprintf(job_workers, 1, "insert read job connection [fd = %d] to epoll\n", read_job_connection->fd);

  job_reader = PipeJobReader{read_job_fd};
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
    ++vk::singleton<SharedMemoryManager>::get().get_stats().errors_pipe_server_write;
    return "Can't write job reply to the pipe";
  }
  ++vk::singleton<SharedMemoryManager>::get().get_stats().jobs_replied;
  reply_was_sent = true;
  return nullptr;
}

void JobWorkerServer::try_store_job_response_error(const char *error_msg, int error_code) {
  php_assert(running_job);
  if (reply_was_sent) {
    return;
  }

  auto &memory_manager = vk::singleton<job_workers::SharedMemoryManager>::get();
  auto *response_memory = memory_manager.acquire_shared_message<job_workers::JobSharedMessage>();
  if (!response_memory) {
    log_server_error("Can't store job response error: not enough shared memory");
    return;
  }

  response_memory->instance = create_error_on_other_memory(error_code, error_msg, response_memory->resource);
  if (const char *err = send_job_reply(response_memory)) {
    memory_manager.release_shared_message(response_memory);
    log_server_error("Can't store job response: %s", err);
  } else {
    memory_manager.detach_shared_message_from_this_proc(response_memory);
  }
}

} // namespace job_workers
