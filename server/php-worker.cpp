// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include <cassert>
#include <poll.h>

#include "common/precise-time.h"
#include "common/rpc-error-codes.h"
#include "common/wrappers/overloaded.h"
#include "net/net-connections.h"
#include "runtime/curl.h"
#include "runtime/job-workers/job-interface.h"
#include "runtime/rpc.h"
#include "server/curl-adaptor.h"
#include "server/database-drivers/adaptor.h"
#include "server/database-drivers/request.h"
#include "server/job-workers/job-stats.h"
#include "server/job-workers/job-worker-server.h"
#include "server/php-engine.h"
#include "server/php-lease.h"
#include "server/php-mc-connections.h"
#include "server/php-sql-connections.h"
#include "server/php-worker.h"
#include "server/server-stats.h"

PhpWorker *active_worker = nullptr;

double PhpWorker::enter_lifecycle() noexcept {
  if (finish_time < precise_now + 0.01) {
    terminate(0, script_error_t::timeout, "timeout");
  }
  on_wakeup();

  paused = false;
  do {
    switch (state) {
      case phpq_try_start:
        state_try_start();
        break;

      case phpq_init_script:
        state_init_script();
        break;

      case phpq_run:
        state_run();
        break;

      case phpq_free_script:
        state_free_script();
        break;

      case phpq_finish:
        state_finish();
        return 0;
    }
    get_utime_monotonic();
  } while (!paused);

  assert(conn->status == conn_wait_net);
  return get_timeout();
}

void PhpWorker::terminate(int flag, script_error_t terminate_reason_, const char *error_message_) noexcept {
  terminate_flag = true;
  terminate_reason = terminate_reason_;
  error_message = error_message_;
  if (flag) {
    vkprintf(0, "php_worker_terminate\n");
    conn = nullptr;
  }
}

void PhpWorker::on_wakeup() noexcept {
  if (waiting) {
    if (wakeup_flag || (wakeup_time != 0 && wakeup_time <= precise_now)) {
      waiting = 0;
      wakeup_time = 0;
      //      php_script_query_answered (php_script);
    }
  }
  wakeup_flag = 0;
}

void PhpWorker::state_try_start() noexcept {
  if (terminate_flag) {
    state = phpq_finish;
    return;
  }

  if (php_worker_run_flag) { // put connection into pending_http_query
    vkprintf(2, "php script [req_id = %016llx] is waiting\n", req_id);

    auto *pending_q = reinterpret_cast<conn_query *>(malloc(sizeof(conn_query)));

    pending_q->custom_type = 0;
    pending_q->outbound = (connection *)&pending_http_queue;
    assert(conn != nullptr);
    pending_q->requester = conn;

    pending_q->cq_type = &pending_cq_func;
    pending_q->timer.wakeup_time = finish_time;

    insert_conn_query(pending_q);

    conn->status = conn_wait_net;

    paused = true;
    return;
  }

  php_worker_run_flag = 1;
  state = phpq_init_script;
}

void PhpWorker::state_init_script() noexcept {
  double timeout = finish_time - precise_now - 0.01;
  if (terminate_flag) {
    state = phpq_finish;
    return;
  }

  if (force_clear_sql_connection && sql_target_id != -1) {
    connection *c, *tmp;
    for (c = Targets[sql_target_id].first_conn; c != (connection *)(Targets + sql_target_id);) {
      tmp = c->next;
      fail_connection(c, -17); // -17 for no error
      c = tmp;
    }
    Targets[sql_target_id].next_reconnect = 0;
    Targets[sql_target_id].next_reconnect_timeout = 0;
    create_new_connections(&Targets[sql_target_id]);
  }

  get_utime_monotonic();
  start_time = precise_now;
  vkprintf(1, "START php script [req_id = %016llx]\n", req_id);
  assert(active_worker == nullptr);
  active_worker = this;
  vk::singleton<ServerStats>::get().set_running_worker_status();

  // init memory allocator for queries
  php_queries_start();

  script_t *script = get_script();
  dl_assert(script != nullptr, "failed to get script");
  if (php_script == nullptr) {
    php_script = new PhpScript(max_memory, oom_handling_memory_ratio, 8 << 20);
  }
  dl::init_critical_section();
  php_script->init(script, data);
  php_script->set_timeout(timeout);
  state = phpq_run;
}

void php_worker_run_rpc_send_query(int32_t request_id, const net_queries_data::rpc_send &query) {
  int connection_id = query.host_num;
  slot_id_t slot_id = request_id;
  if (connection_id < 0 || connection_id >= MAX_TARGETS) {
    on_net_event(create_rpc_error_event(slot_id, TL_ERROR_INVALID_CONNECTION_ID, "Invalid connection_id (1)", nullptr));
    return;
  }
  conn_target_t *target = &Targets[connection_id];
  connection *conn = get_target_connection(target, 0);

  if (conn != nullptr) {
    send_rpc_query(conn, TL_RPC_INVOKE_REQ, slot_id, reinterpret_cast<int *>(query.request), query.request_size);
    conn->last_query_sent_time = precise_now;
  } else {
    int new_conn_cnt = create_new_connections(target);
    if (new_conn_cnt <= 0 && get_target_connection(target, 1) == nullptr) {
      on_net_event(create_rpc_error_event(slot_id, TL_ERROR_NO_CONNECTIONS_IN_RPC_CLIENT, "Failed to establish connection [probably reconnect timeout is not expired]", nullptr));
      return;
    }

    command_t *command = create_command_net_writer(query.request, query.request_size, &command_net_write_rpc_base, slot_id);
    double timeout = fix_timeout(query.timeout_ms * 0.001) + precise_now;
    create_delayed_send_query(target, command, timeout);
  }
}

void php_worker_run_net_queue(PhpWorker *worker __attribute__((unused))) {
  net_query_t *query;
  while ((query = pop_net_query()) != nullptr) {
    // no other types of query are currently supported
    std::visit(overloaded{
                 [&](const net_queries_data::rpc_send &data) {
                   php_worker_run_rpc_send_query(query->slot_id, data);
                   free_rpc_send_query(data);
                 },
                 [&](database_drivers::Request *data) {
                   php_assert(query->slot_id == data->request_id);
                   vk::singleton<database_drivers::Adaptor>::get().process_external_db_request_net_query(std::unique_ptr<database_drivers::Request>(data));
                 },
                 [&](const curl_async::CurlRequest &request) {
                   php_assert(query->slot_id == request.request_id);
                   vk::singleton<curl_async::CurlAdaptor>::get().process_request_net_query(request);
                 }},
               query->data);
  }
}

void PhpWorker::state_run() noexcept {
  int f = 1;
  while (f) {
    if (terminate_flag) {
      php_script->terminate(error_message, terminate_reason);
    }

    //    fprintf (stderr, "state = %d, f = %d\n", php_script_get_state (php_script), f);
    switch (php_script->state) {
      case run_state_t::ready: {
        vk::singleton<ServerStats>::get().set_running_worker_status();
        if (waiting) {
          f = 0;
          paused = true;
          vk::singleton<ServerStats>::get().set_wait_net_worker_status();
          conn->status = conn_wait_net;
          vkprintf(2, "php_script_iterate [req_id = %016llx] delayed due to net events\n", req_id);
          break;
        }
        vkprintf(2, "before php_script_iterate [req_id = %016llx] (before swap context)\n", req_id);
        php_script->iterate();
        vkprintf(2, "after php_script_iterate [req_id = %016llx] (after swap context)\n", req_id);
        wait(0); // check for net events
        break;
      }
      case run_state_t::query: {
        if (waiting) {
          f = 0;
          paused = true;
          vk::singleton<ServerStats>::get().set_wait_net_worker_status();
          conn->status = conn_wait_net;
          vkprintf(2, "query [req_id = %016llx] delayed due to net events\n", req_id);
          break;
        }
        vkprintf(2, "got query [req_id = %016llx]\n", req_id);
        run_query();
        php_worker_run_net_queue(this);
        wait(0); // check for net events
        break;
      }
      case run_state_t::query_running: {
        vkprintf(2, "paused due to query [req_id = %016llx]\n", req_id);
        f = 0;
        paused = true;
        vk::singleton<ServerStats>::get().set_wait_net_worker_status();
        break;
      }
      case run_state_t::error: {
        vkprintf(2, "php script [req_id = %016llx]: ERROR (probably timeout)\n", req_id);
        if (dl::is_malloc_replaced()) {
          // in case the error happened when malloc was replaced
          dl::rollback_malloc_replacement();
        }
        php_script->finish();

        if (conn != nullptr) {
          switch (mode) {
            case http_worker:
              if (!flushed_http_connection) {
                http_return(conn, "ERROR", 5);
              }
              break;
            case rpc_worker:
              if (!rpc_stored) {
                server_rpc_error(conn, req_id, -504, php_script->error_message);
              }
              break;
            case job_worker: {
              const char *error = php_script->error_message;
              int error_code = job_workers::server_php_script_error_offset - static_cast<int>(php_script->error_type);
              auto &job_server = vk::singleton<job_workers::JobWorkerServer>::get();
              if (job_server.reply_is_expected()) {
                job_server.store_job_response_error(error, error_code);
              }
              break;
            }
            default:;
          }
        }

        state = phpq_free_script;
        f = 0;
        break;
      }
      case run_state_t::finished: {
        vkprintf(2, "php script [req_id = %016llx]: OK (still can return RPC_ERROR)\n", req_id);
        script_result *res = php_script->res;
        set_result(res);
        php_script->finish();

        state = phpq_free_script;
        f = 0;
        break;
      }
      default:
        assert("php_worker_run: unexpected state" && 0);
    }
    get_utime_monotonic();
  }
}

void PhpWorker::wait(int timeout_ms) noexcept {
  if (waiting) { // first timeout is used!!
    return;
  }
  waiting = 1;
  if (timeout_ms == 0) {
    int new_net_events_cnt = epoll_fetch_events(0);
    // TODO: maybe we have to wait for timers too
    if (epoll_event_heap_size() > 0) {
      vkprintf(2, "paused for some nonblocking net activity [req_id = %016llx]\n", req_id);
      wakeup();
      return;
    } else {
      assert(new_net_events_cnt == 0);
      waiting = 0;
    }
  } else {
    if (!net_events_empty()) {
      waiting = 0;
    } else {
      vkprintf(2, "paused for some blocking net activity [req_id = %016llx] [timeout = %.3lf]\n", req_id, timeout_ms * 0.001);
      wakeup_time = get_utime_monotonic() + timeout_ms * 0.001;
    }
  }
}

void PhpWorker::answer_query(void *ans) noexcept {
  assert(ans != nullptr);
  auto *q_base = php_script->query;
  q_base->ans = ans;
  php_script->query_answered();
}

void PhpWorker::wakeup() noexcept {
  if (!wakeup_flag) {
    assert(conn != nullptr);
    put_event_into_heap_tail(conn->ev, 1);
    wakeup_flag = 1;
  }
}

void PhpWorker::run_query() noexcept {
  auto *q_base = php_script->query;

  qmem_free_ptrs();

  query_stats.q_id = query_stats_id;

  q_base->run(this);
}

void PhpWorker::set_result(script_result *res) noexcept {
  if (conn != nullptr) {
    if (mode == http_worker) {
      if (res == nullptr) {
        http_return(conn, "OK", 2);
      } else {
        write_out(&conn->Out, res->headers, res->headers_len);
        write_out(&conn->Out, res->body, res->body_len);
      }
    } else if (mode == rpc_worker) {
      if (!rpc_stored) {
        server_rpc_error(conn, req_id, -505, "Nothing stored");
      }
    } else if (mode == once_worker) {
      assert(write(1, res->body, (size_t)res->body_len) == res->body_len);
      run_once_return_code = res->exit_code;
    } else if (mode == job_worker) {
      auto &job_server = vk::singleton<job_workers::JobWorkerServer>::get();
      if (job_server.reply_is_expected()) {
        job_server.store_job_response_error("Nothing replied", job_workers::server_nothing_replied_error);
      }
    }
  }
}

void PhpWorker::state_free_script() noexcept {
  php_worker_run_flag = 0;
  int f = 0;

  get_utime_monotonic();
  double worked = precise_now - start_time;
  double waited = start_time - init_time;

  assert(active_worker == this);
  active_worker = nullptr;
  vkprintf(1, "FINISH php script [query worked = %.5lf] [query waited for start = %.5lf] [req_id = %016llx]\n", worked, waited, req_id);
  vk::singleton<ServerStats>::get().set_idle_worker_status();
  if (mode == once_worker) {
    static int left = run_once_count;
    if (!--left) {
      turn_sigterm_on();
    }
  }
  if (worked + waited > 1.0) {
    vkprintf(1, "ATTENTION php script [query worked = %.5lf] [query waited for start = %.5lf] [req_id = %016llx]\n", worked, waited, req_id);
  }

  while (pending_http_queue.first_query != (conn_query *)&pending_http_queue && !f) {
    // TODO: is it correct to do it?
    conn_query *q = pending_http_queue.first_query;
    f = q->requester != nullptr && q->requester->generation == q->req_generation;
    delete_pending_query(q);
  }

  php_queries_finish();
  php_script->disable_timeout();
  php_script->clear();

  static int finished_queries = 0;
  if ((++finished_queries) % queries_to_recreate_script == 0
      || (!use_madvise_dontneed && php_script->memory_get_total_usage() > memory_used_to_recreate_script)) {
    delete php_script;
    php_script = nullptr;
    finished_queries = 0;
  }

  state = phpq_finish;
}

void PhpWorker::state_finish() noexcept {
  vkprintf(2, "free php script [req_id = %016llx]\n", req_id);
  clear_shared_job_messages(); // it's here because `phpq_free_script` state is skipped when worker->terminate_flag == true
  lease_on_worker_finish(this);
}

double PhpWorker::get_timeout() const noexcept {
  double new_wakeup_time = finish_time;
  if (wakeup_time != 0 && wakeup_time < new_wakeup_time) {
    new_wakeup_time = wakeup_time;
  }
  double time_left = new_wakeup_time - precise_now;
  if (time_left <= 0) {
    time_left = 0.00001;
  }
  return time_left;
}

PhpWorker::PhpWorker(php_worker_mode_t mode_, connection *c, http_query_data *http_data, rpc_query_data *rpc_data, job_query_data *job_data,
                       long long int req_id_, double timeout)
  : conn(c)
  , data(php_query_data_create(http_data, rpc_data, job_data))
  , paused(false)
  , flushed_http_connection(false)
  , terminate_flag(false)
  , terminate_reason(script_error_t::unclassified_error)
  , error_message("no error")
  , waiting(0)
  , wakeup_flag(0)
  , wakeup_time(0)
  , init_time(precise_now)
  , finish_time(precise_now + timeout)
  , state(phpq_try_start)
  , mode(mode_)
  , req_id(req_id_)
{
  assert(c != nullptr);
  if (conn->target) {
    target_fd = static_cast<int>(conn->target - Targets);
  } else {
    target_fd = -1;
  }
  vkprintf(2, "create php script [req_id = %016llx]\n", req_id);
}

PhpWorker::~PhpWorker() {
  php_query_data_free(data);
  data = nullptr;
}
