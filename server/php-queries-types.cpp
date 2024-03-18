// Compiler for PHP (aka KPHP)
// Copyright (c) 2021 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include <poll.h>

#include "common/kprintf.h"
#include "server/database-drivers/adaptor.h"
#include "server/database-drivers/connector.h"
#include "server/database-drivers/response.h"
#include "server/php-engine.h"
#include "server/php-mc-connections.h"
#include "server/php-queries-types.h"
#include "server/php-runner.h"
#include "server/php-sql-connections.h"
#include "server/php-worker.h"

void php_query_x2_t::run([[maybe_unused]] PhpWorker *worker) noexcept {
  query_stats.desc = "PHPQX2";

  php_script->query_readed();

  //  worker->conn->status = conn_wait_net;
  //  worker->conn->pending_queries = 1;

  static php_query_x2_answer_t res;
  res.x2 = val * val;

  ans = &res;

  php_script->query_answered();
}

void php_query_rpc_answer::run(PhpWorker *worker) noexcept {
  query_stats.desc = "RPC_ANSWER";

  if (worker->mode == rpc_worker) {
    connection *c = worker->conn;
    int *q = (int *)data;
    int qsize = data_len;
    tvkprintf(php_connections, 3, "going to send %d bytes as an answer [req_id = %016llx]\n", qsize, worker->req_id);
    send_rpc_query(c, q[2] == 0 ? TL_RPC_REQ_RESULT : TL_RPC_REQ_ERROR, worker->req_id, q, qsize);
  }
  php_script->query_readed();
  php_script->query_answered();
}

void php_query_connect_t::run([[maybe_unused]] PhpWorker *worker) noexcept {
  query_stats.desc = "CONNECT";

  php_script->query_readed();

  static php_query_connect_answer_t res;

  switch (protocol) {
    case protocol_type::memcached:
      res.connection_id = get_target(host, port, &memcache_ct);
      break;
    case protocol_type::mysqli:
      res.connection_id = sql_target_id;
      break;
    case protocol_type::rpc:
      res.connection_id = get_target(host, port, &rpc_ct);
      break;
    default:
      assert("unknown protocol" && 0);
  }

  ans = &res;

  php_script->query_answered();
}

external_driver_connect::external_driver_connect(std::unique_ptr<database_drivers::Connector> &&connector)
  : connector(std::move(connector)){};

void external_driver_connect::run(PhpWorker *worker __attribute__((unused))) noexcept {
  query_stats.desc = "CONNECT_EXTERNAL_DRIVER";

  php_script->query_readed();

  static php_query_connect_answer_t res;

  assert(connector);
  int id = vk::singleton<database_drivers::Adaptor>::get().register_connector(std::move(connector));
  res.connection_id = id;

  ans = &res;

  php_script->query_answered();
}

void php_query_wait_t::run(PhpWorker *worker) noexcept {
  query_stats.desc = "WAIT_NET";
  php_script->query_readed();
  php_script->query_answered();

  worker->wait(timeout_ms);
}

int php_worker_http_load_post_impl(PhpWorker *worker, char *buf, int min_len, int max_len) {
  connection *c = worker->conn;
  double precise_now = get_utime_monotonic();

  //  fprintf (stderr, "Trying to load data of len [%d;%d] at %.6lf\n", min_len, max_len, precise_now - worker->start_time);

  if (worker->finish_time < precise_now + 0.01) {
    return -1;
  }

  if (c == nullptr || c->error) {
    return -1;
  }

  assert(!c->crypto);
  assert(c->basic_type != ct_pipe);
  assert(min_len <= max_len);

  int read = 0;
  int have_bytes = get_total_ready_bytes(&c->In);
  if (have_bytes > 0) {
    if (have_bytes > max_len) {
      have_bytes = max_len;
    }
    assert(read_in(&c->In, buf, have_bytes) == have_bytes);
    read += have_bytes;
  }

  pollfd poll_fds;
  poll_fds.fd = c->fd;
  poll_fds.events = POLLIN | POLLPRI;

  while (read < min_len) {
    precise_now = get_utime_monotonic();

    double left_time = worker->finish_time - precise_now;
    assert(left_time < 2000000.0);

    if (left_time < 0.01) {
      return -1;
    }

    int r = poll(&poll_fds, 1, (int)(left_time * 1000 + 1));
    int err = errno;
    if (r > 0) {
      assert(r == 1);

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

void php_query_http_load_post_t::run(PhpWorker *worker) noexcept {
  query_stats.desc = "HTTP_LOAD_POST";

  php_script->query_readed();

  static php_query_http_load_post_answer_t res;
  res.loaded_bytes = php_worker_http_load_post_impl(worker, buf, min_len, max_len);
  ans = &res;

  php_script->query_answered();

  if (res.loaded_bytes < 0) {
    // TODO we need to close connection. Do we need to pass 1 as second parameter?
    worker->terminate(1, script_error_t::post_data_loading_error, "error during loading big post data");
  }
}

void php_net_query_packet_t::run(PhpWorker *worker) noexcept {
  query_stats.desc = "NET";

  switch (protocol) {
    case protocol_type::memcached:
      php_worker_run_mc_query_packet(worker, this);
      break;
    case protocol_type::mysqli:
      php_worker_run_sql_query_packet(worker, this);
      break;
    default:
      assert(0);
  }
}
