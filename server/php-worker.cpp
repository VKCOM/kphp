// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include <cassert>
#include <poll.h>

#include "common/precise-time.h"
#include "common/rpc-error-codes.h"
#include "net/net-connections.h"
#include "runtime/rpc.h"
#include "runtime/job-workers/job-interface.h"
#include "server/job-workers/job-stats.h"
#include "server/job-workers/job-worker-server.h"
#include "server/php-engine.h"
#include "server/php-lease.h"
#include "server/php-mc-connections.h"
#include "server/php-sql-connections.h"
#include "server/php-worker.h"

php_worker *active_worker = nullptr;

php_worker *php_worker_create(php_worker_mode_t mode, connection *c, http_query_data *http_data, rpc_query_data *rpc_data, job_query_data *job_data,
                              long long req_id, double timeout) {
  auto worker = reinterpret_cast<php_worker *>(malloc(sizeof(php_worker)));

  worker->data = php_query_data_create(http_data, rpc_data, job_data);
  worker->conn = c;
  assert (c != nullptr);

  worker->state = phpq_try_start;
  worker->mode = mode;

  worker->init_time = precise_now;
  worker->finish_time = precise_now + timeout;

  worker->paused = false;
  worker->terminate_flag = false;
  worker->terminate_reason = script_error_t::unclassified_error;
  worker->error_message = "no error";

  worker->waiting = 0;
  worker->wakeup_flag = 0;
  worker->wakeup_time = 0;

  worker->req_id = req_id;

  if (worker->conn->target) {
    worker->target_fd = static_cast<int>(worker->conn->target - Targets);
  } else {
    worker->target_fd = -1;
  }

  vkprintf (2, "create php script [req_id = %016llx]\n", req_id);

  return worker;
}

void php_worker_free(php_worker *worker) {
  if (worker == nullptr) {
    return;
  }

  php_query_data_free(worker->data);
  worker->data = nullptr;

  free(worker);
}

double php_worker_main(php_worker *worker) {
  if (worker->finish_time < precise_now + 0.01) {
    php_worker_terminate(worker, 0, script_error_t::timeout, "timeout");
  }
  php_worker_on_wakeup(worker);

  worker->paused = false;
  do {
    switch (worker->state) {
      case phpq_try_start:
        php_worker_try_start(worker);
        break;

      case phpq_init_script:
        php_worker_init_script(worker);
        break;

      case phpq_run:
        php_worker_run(worker);
        break;

      case phpq_free_script:
        php_worker_free_script(worker);
        break;

      case phpq_finish:
        php_worker_finish(worker);
        return 0;
    }
    get_utime_monotonic();
  } while (!worker->paused);

  assert (worker->conn->status == conn_wait_net);
  return php_worker_get_timeout(worker);
}

void php_worker_terminate(php_worker *worker, int flag, script_error_t terminate_reason, const char *error_message) {
  worker->terminate_flag = true;
  worker->terminate_reason = terminate_reason;
  worker->error_message = error_message;
  if (flag) {
    vkprintf(0, "php_worker_terminate\n");
    worker->conn = nullptr;
  }
}

void php_worker_on_wakeup(php_worker *worker) {
  if (worker->waiting) {
    if (worker->wakeup_flag ||
        (worker->wakeup_time != 0 && worker->wakeup_time <= precise_now)) {
      worker->waiting = 0;
      worker->wakeup_time = 0;
//      php_script_query_answered (php_script);
    }
  }
  worker->wakeup_flag = 0;
}

/** trying to start query **/
void php_worker_try_start(php_worker *worker) {

  if (worker->terminate_flag) {
    worker->state = phpq_finish;
    return;
  }

  if (php_worker_run_flag) { // put connection into pending_http_query
    vkprintf (2, "php script [req_id = %016llx] is waiting\n", worker->req_id);

    auto pending_q = reinterpret_cast<conn_query *>(malloc(sizeof(conn_query)));

    pending_q->custom_type = 0;
    pending_q->outbound = (connection *)&pending_http_queue;
    assert (worker->conn != nullptr);
    pending_q->requester = worker->conn;

    pending_q->cq_type = &pending_cq_func;
    pending_q->timer.wakeup_time = worker->finish_time;

    insert_conn_query(pending_q);

    worker->conn->status = conn_wait_net;

    worker->paused = true;
    return;
  }

  php_worker_run_flag = 1;
  worker->state = phpq_init_script;
}

void php_worker_init_script(php_worker *worker) {
  double timeout = worker->finish_time - precise_now - 0.01;
  if (worker->terminate_flag) {
    worker->state = phpq_finish;
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
  worker->start_time = precise_now;
  vkprintf (1, "START php script [req_id = %016llx]\n", worker->req_id);
  assert (active_worker == nullptr);
  active_worker = worker;
  running_server_status();

  //init memory allocator for queries
  php_queries_start();

  script_t *script = get_script();
  dl_assert (script != nullptr, "failed to get script");
  if (php_script == nullptr) {
    php_script = php_script_create((size_t)max_memory, (size_t)(8 << 20));
  }
  php_script_init(php_script, script, worker->data);
  php_script_set_timeout(timeout);
  worker->state = phpq_run;
}

void php_worker_run_rpc_send_query(net_query_t *query) {
  int connection_id = query->host_num;
  slot_id_t slot_id = query->slot_id;
  if (connection_id < 0 || connection_id >= MAX_TARGETS) {
    on_net_event(create_rpc_error_event(slot_id, TL_ERROR_INVALID_CONNECTION_ID, "Invalid connection_id (1)", nullptr));
    return;
  }
  conn_target_t *target = &Targets[connection_id];
  connection *conn = get_target_connection(target, 0);

  if (conn != nullptr) {
    send_rpc_query(conn, TL_RPC_INVOKE_REQ, slot_id, (int *)query->request, query->request_size);
    conn->last_query_sent_time = precise_now;
  } else {
    int new_conn_cnt = create_new_connections(target);
    if (new_conn_cnt <= 0 && get_target_connection(target, 1) == nullptr) {
      on_net_event(create_rpc_error_event(slot_id, TL_ERROR_NO_CONNECTIONS, "Failed to establish connection [probably reconnect timeout is not expired]", nullptr));
      return;
    }

    command_t *command = create_command_net_writer(query->request, query->request_size, &command_net_write_rpc_base, slot_id);
    double timeout = fix_timeout(query->timeout_ms * 0.001) + precise_now;
    create_delayed_send_query(target, command, timeout);
  }
}

void php_worker_run_net_queue(php_worker *worker __attribute__((unused))) {
  net_query_t *query;
  while ((query = pop_net_query()) != nullptr) {
    //no other types of query are currently supported
    php_worker_run_rpc_send_query(query);
    free_net_query(query);
  }
}

void php_worker_run(php_worker *worker) {
  int f = 1;
  while (f) {
    if (worker->terminate_flag) {
      php_script_terminate(php_script, worker->error_message, worker->terminate_reason);
    }

//    fprintf (stderr, "state = %d, f = %d\n", php_script_get_state (php_script), f);
    switch (php_script_get_state(php_script)) {
      case run_state_t::ready: {
        running_server_status();
        if (worker->waiting) {
          f = 0;
          worker->paused = true;
          wait_net_server_status();
          worker->conn->status = conn_wait_net;
          vkprintf (2, "php_script_iterate [req_id = %016llx] delayed due to net events\n", worker->req_id);
          break;
        }
        vkprintf (2, "before php_script_iterate [req_id = %016llx] (before swap context)\n", worker->req_id);
        php_script_iterate(php_script);
        vkprintf (2, "after php_script_iterate [req_id = %016llx] (after swap context)\n", worker->req_id);
        php_worker_wait(worker, 0); //check for net events
        break;
      }
      case run_state_t::query: {
        if (worker->waiting) {
          f = 0;
          worker->paused = true;
          wait_net_server_status();
          worker->conn->status = conn_wait_net;
          vkprintf (2, "query [req_id = %016llx] delayed due to net events\n", worker->req_id);
          break;
        }
        vkprintf (2, "got query [req_id = %016llx]\n", worker->req_id);
        php_worker_run_query(worker);
        php_worker_run_net_queue(worker);
        php_worker_wait(worker, 0); //check for net events
        break;
      }
      case run_state_t::query_running: {
        vkprintf (2, "paused due to query [req_id = %016llx]\n", worker->req_id);
        f = 0;
        worker->paused = true;
        wait_net_server_status();
        break;
      }
      case run_state_t::error: {
        vkprintf (2, "php script [req_id = %016llx]: ERROR (probably timeout)\n", worker->req_id);
        php_script_finish(php_script);

        if (worker->conn != nullptr) {
          switch (worker->mode) {
            case http_worker:
              http_return(worker->conn, "ERROR", 5);
              break;
            case rpc_worker:
              if (!rpc_stored) {
                server_rpc_error(worker->conn, worker->req_id, -504, php_script_get_error(php_script));
              }
              break;
            case job_worker: {
              const char *error = php_script_get_error(php_script);
              int error_code = job_workers::server_php_script_error_offset - static_cast<int>(php_script_get_error_type(php_script));
              vk::singleton<job_workers::JobWorkerServer>::get().try_store_job_response_error(error, error_code);
              break;
            }
            default:;
          }
        }

        worker->state = phpq_free_script;
        f = 0;
        break;
      }
      case run_state_t::finished: {
        vkprintf (2, "php script [req_id = %016llx]: OK (still can return RPC_ERROR)\n", worker->req_id);
        script_result *res = php_script_get_res(php_script);
        php_worker_set_result(worker, res);
        php_script_finish(php_script);

        worker->state = phpq_free_script;
        f = 0;
        break;
      }
      default:
        assert ("php_worker_run: unexpected state" && 0);
    }
    get_utime_monotonic();
  }
}

void php_worker_wait(php_worker *worker, int timeout_ms) {
  if (worker->waiting) { // first timeout is used!!
    return;
  }
  worker->waiting = 1;
  if (timeout_ms == 0) {
    int new_net_events_cnt = epoll_fetch_events(0);
    //TODO: maybe we have to wait for timers too
    if (epoll_event_heap_size() > 0) {
      vkprintf (2, "paused for some nonblocking net activity [req_id = %016llx]\n", worker->req_id);
      php_worker_wakeup(worker);
      return;
    } else {
      assert (new_net_events_cnt == 0);
      worker->waiting = 0;
    }
  } else {
    if (!net_events_empty()) {
      worker->waiting = 0;
    } else {
      vkprintf (2, "paused for some blocking net activity [req_id = %016llx] [timeout = %.3lf]\n", worker->req_id, timeout_ms * 0.001);
      worker->wakeup_time = get_utime_monotonic() + timeout_ms * 0.001;
    }
  }
}

void php_worker_run_rpc_answer_query(php_worker *worker, php_query_rpc_answer *ans) {
  if (worker->mode == rpc_worker) {
    connection *c = worker->conn;
    int *q = (int *)ans->data;
    int qsize = ans->data_len;

    vkprintf (2, "going to send %d bytes as an answer [req_id = %016llx]\n", qsize, worker->req_id);
    send_rpc_query(c, q[2] == 0 ? TL_RPC_REQ_RESULT : TL_RPC_REQ_ERROR, worker->req_id, q, qsize);
  }
  php_script_query_readed(php_script);
  php_script_query_answered(php_script);
}

int php_worker_http_load_post_impl(php_worker *worker, char *buf, int min_len, int max_len) {
  assert (worker != nullptr);

  connection *c = worker->conn;
  double precise_now = get_utime_monotonic();

//  fprintf (stderr, "Trying to load data of len [%d;%d] at %.6lf\n", min_len, max_len, precise_now - worker->start_time);

  if (worker->finish_time < precise_now + 0.01) {
    return -1;
  }

  if (c == nullptr || c->error) {
    return -1;
  }

  assert (!c->crypto);
  assert (c->basic_type != ct_pipe);
  assert (min_len <= max_len);

  int read = 0;
  int have_bytes = get_total_ready_bytes(&c->In);
  if (have_bytes > 0) {
    if (have_bytes > max_len) {
      have_bytes = max_len;
    }
    assert (read_in(&c->In, buf, have_bytes) == have_bytes);
    read += have_bytes;
  }

  pollfd poll_fds;
  poll_fds.fd = c->fd;
  poll_fds.events = POLLIN | POLLPRI;

  while (read < min_len) {
    precise_now = get_utime_monotonic();

    double left_time = worker->finish_time - precise_now;
    assert (left_time < 2000000.0);

    if (left_time < 0.01) {
      return -1;
    }

    int r = poll(&poll_fds, 1, (int)(left_time * 1000 + 1));
    int err = errno;
    if (r > 0) {
      assert (r == 1);

      r = static_cast<int>(recv(c->fd, buf + read, max_len - read, 0));
      err = errno;
/*
      if (r < 0) {
        fprintf (stderr, "Error recv: %m\n");
      } else {
        fprintf (stderr, "Received %d bytes at %.6lf\n", r, precise_now - worker->start_time);
      }
*/
      if (r > 0) {
        read += r;
      } else {
        if (r == 0) {
          return -1;
        }

        if (err != EAGAIN && err != EWOULDBLOCK && err != EINTR) {
          return -1;
        }
      }
    } else {
      if (r == 0) {
        return -1;
      }

//      fprintf (stderr, "Error poll: %m\n");
      if (err != EINTR) {
        return -1;
      }
    }
  }

//  fprintf (stderr, "%d bytes loaded\n", read);
  return read;
}

void php_worker_http_load_post(php_worker *worker, php_query_http_load_post_t *query) {
  php_script_query_readed(php_script);

  static php_query_http_load_post_answer_t res;
  res.loaded_bytes = php_worker_http_load_post_impl(worker, query->buf, query->min_len, query->max_len);
  query->base.ans = &res;

  php_script_query_answered(php_script);

  if (res.loaded_bytes < 0) {
    // TODO we need to close connection. Do we need to pass 1 as second parameter?
    php_worker_terminate(worker, 1, script_error_t::post_data_loading_error, "error during loading big post data");
  }
}

void php_worker_answer_query(php_worker *worker, void *ans) {
  assert (worker != nullptr && ans != nullptr);
  auto q_base = (php_query_base_t *)php_script_get_query(php_script);
  q_base->ans = ans;
  php_script_query_answered(php_script);
}

void php_worker_wakeup(php_worker *worker) {
  if (!worker->wakeup_flag) {
    assert (worker->conn != nullptr);
    put_event_into_heap_tail(worker->conn->ev, 1);
    worker->wakeup_flag = 1;
  }
}

void php_worker_run_query_x2(php_worker *worker __attribute__((unused)), php_query_x2_t *query) {
  php_script_query_readed(php_script);

//  worker->conn->status = conn_wait_net;
//  worker->conn->pending_queries = 1;

  static php_query_x2_answer_t res;
  res.x2 = query->val * query->val;

  query->base.ans = &res;

  php_script_query_answered(php_script);
}

void php_worker_run_query_connect(php_worker *worker __attribute__((unused)), php_query_connect_t *query) {
  php_script_query_readed(php_script);

  static php_query_connect_answer_t res;

  switch (query->protocol) {
    case p_memcached:
      res.connection_id = get_target(query->host, query->port, &memcache_ct);
      break;
    case p_sql:
      res.connection_id = sql_target_id;
      break;
    case p_rpc:
      res.connection_id = get_target(query->host, query->port, &rpc_ct);
      break;
    default:
      assert ("unknown protocol" && 0);
  }

  query->base.ans = &res;

  php_script_query_answered(php_script);
}

void php_worker_run_net_query_packet(php_worker *worker, php_net_query_packet_t *query) {
  switch (query->protocol) {
    case p_memcached:
      php_worker_run_mc_query_packet(worker, query);
      break;
    case p_sql:
      php_worker_run_sql_query_packet(worker, query);
      break;
    default:
      assert (0);
  }
}

void php_worker_run_net_query(php_worker *worker, php_query_base_t *q_base) {
  switch (q_base->type & 0xFFFF) {
    case NETQ_PACKET:
      php_worker_run_net_query_packet(worker, (php_net_query_packet_t *)q_base);
      break;
    default:
      assert ("unknown net_query type" && 0);
  }
}

void php_worker_run_query(php_worker *worker) {
  auto q_base = (php_query_base_t *)php_script_get_query(php_script);

  qmem_free_ptrs();

  query_stats.q_id = query_stats_id;
  switch ((unsigned int)q_base->type & 0xFFFF0000) {
    case PHPQ_X2:
      query_stats.desc = "PHPQX2";
      php_worker_run_query_x2(worker, (php_query_x2_t *)q_base);
      break;
    case PHPQ_RPC_ANSWER:
      query_stats.desc = "RPC_ANSWER";
      php_worker_run_rpc_answer_query(worker, (php_query_rpc_answer *)q_base);
      break;
    case PHPQ_CONNECT:
      query_stats.desc = "CONNECT";
      php_worker_run_query_connect(worker, (php_query_connect_t *)q_base);
      break;
    case PHPQ_NETQ:
      query_stats.desc = "NET";
      php_worker_run_net_query(worker, q_base);
      break;
    case PHPQ_WAIT:
      query_stats.desc = "WAIT_NET";
      php_script_query_readed(php_script);
      php_script_query_answered(php_script);
      php_worker_wait(worker, ((php_query_wait_t *)q_base)->timeout_ms);
      break;
    case PHPQ_HTTP_LOAD_POST:
      query_stats.desc = "HTTP_LOAD_POST";
      php_worker_http_load_post(worker, (php_query_http_load_post_t *)q_base);
      break;
    default:
      assert ("unknown php_query type" && 0);
  }
}

void php_worker_set_result(php_worker *worker, script_result *res) {
  if (worker->conn != nullptr) {
    if (worker->mode == http_worker) {
      if (res == nullptr) {
        http_return(worker->conn, "OK", 2);
      } else {
        write_out(&worker->conn->Out, res->headers, res->headers_len);
        write_out(&worker->conn->Out, res->body, res->body_len);
      }
    } else if (worker->mode == rpc_worker) {
      if (!rpc_stored) {
        server_rpc_error(worker->conn, worker->req_id, -505, "Nothing stored");
      }
    } else if (worker->mode == once_worker) {
      assert (write(1, res->body, (size_t)res->body_len) == res->body_len);
      run_once_return_code = res->exit_code;
    } else if (worker->mode == job_worker) {
      auto &job_server = vk::singleton<job_workers::JobWorkerServer>::get();
      if (job_server.reply_is_expected()) {
        job_server.try_store_job_response_error("Nothing replied", job_workers::server_nothing_replied_error);
      }
    }
  }
}

void php_worker_free_script(php_worker *worker) {
  php_worker_run_flag = 0;
  int f = 0;

  get_utime_monotonic();
  double worked = precise_now - worker->start_time;
  double waited = worker->start_time - worker->init_time;

  assert (active_worker == worker);
  active_worker = nullptr;
  vkprintf (1, "FINISH php script [query worked = %.5lf] [query waited for start = %.5lf] [req_id = %016llx]\n", worked, waited, worker->req_id);
  idle_server_status();
  custom_server_status("<none>", 6);
  server_status_rpc(0, 0, precise_now);
  if (worker->mode == once_worker) {
    static int left = run_once_count;
    if (!--left) {
      turn_sigterm_on();
    }
  }
  if (worked + waited > 1.0) {
    vkprintf (1, "ATTENTION php script [query worked = %.5lf] [query waited for start = %.5lf] [req_id = %016llx]\n", worked, waited, worker->req_id);
  }

  while (pending_http_queue.first_query != (conn_query *)&pending_http_queue && !f) {
    //TODO: is it correct to do it?
    conn_query *q = pending_http_queue.first_query;
    f = q->requester != nullptr && q->requester->generation == q->req_generation;
    delete_pending_query(q);
  }

  php_queries_finish();
  php_script_clear(php_script);

  static int finished_queries = 0;
  if ((++finished_queries) % queries_to_recreate_script == 0
      || (!use_madvise_dontneed && php_script_memory_get_total_usage(php_script) > memory_used_to_recreate_script)) {
    php_script_free(php_script);
    php_script = nullptr;
    finished_queries = 0;
  }

  worker->state = phpq_finish;
}


void php_worker_finish(php_worker *worker) {
  vkprintf (2, "free php script [req_id = %016llx]\n", worker->req_id);
  clear_shared_job_messages(); // it's here because `phpq_free_script` state is skipped when worker->terminate_flag == true
  lease_on_worker_finish(worker);
  php_worker_free(worker);
}

double php_worker_get_timeout(php_worker *worker) {
  double wakeup_time = worker->finish_time;
  if (worker->wakeup_time != 0 && worker->wakeup_time < wakeup_time) {
    wakeup_time = worker->wakeup_time;
  }
  double time_left = wakeup_time - precise_now;
  if (time_left <= 0) {
    time_left = 0.00001;
  }
  return time_left;
}
