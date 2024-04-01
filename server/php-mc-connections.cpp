// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "server/php-mc-connections.h"

#include "net/net-connections.h"
#include "net/net-memcache-client.h"

#include "server/php-engine.h"
#include "server/php-queries.h"
#include "server/php-runner.h"
#include "server/php-worker.h"

void data_read_conn(data_reader_t *reader, void *dest) {
  reader->readed = 1;
  read_in(&((connection *)(reader->extra))->In, dest, reader->len);
}

data_reader_t *create_data_reader(connection *c, int data_len) {
  static data_reader_t reader;

  reader.readed = 0;
  reader.len = data_len;
  reader.extra = c;

  reader.read = data_read_conn;

  return &reader;
}

int mc_query_value(conn_query *q, data_reader_t *reader) {
  auto *ansgen = (mc_ansgen_t *)q->extra;
  ansgen->func->value(ansgen, reader);

  int err = pnet_query_check(q);
  return err;
}

int mc_query_other(conn_query *q, data_reader_t *reader) {
  auto *ansgen = (mc_ansgen_t *)q->extra;
  ansgen->func->other(ansgen, reader);

  return pnet_query_check(q);
}

int mc_query_version(conn_query *q, data_reader_t *reader) {
  auto *ansgen = (mc_ansgen_t *)q->extra;
  ansgen->func->version(ansgen, reader);

  return pnet_query_check(q);
}

int mc_query_end(conn_query *q) {
  auto *ansgen = (mc_ansgen_t *)q->extra;
  ansgen->func->end(ansgen);

  return pnet_query_check(q);
}

int mc_query_xstored(conn_query *q, int is_stored) {
  auto *ansgen = (mc_ansgen_t *)q->extra;
  ansgen->func->xstored(ansgen, is_stored);

  return pnet_query_check(q);
}

int mc_query_error(conn_query *q) {
  auto *ansgen = (net_ansgen_t *)q->extra;
  ansgen->func->error(ansgen, "some protocol error");

  return pnet_query_check(q);
}

int memcache_client_check_ready(connection *c) {
  if (c->status == conn_connecting) {
    return c->ready = cr_ok;
  }

  /*assert (c->status != conn_none);*/
  if (c->status == conn_none) {
    return c->ready = cr_notyet;
  }
  if (c->status == conn_error || c->ready == cr_failed) {
    return c->ready = cr_failed;
  }
  return c->ready = cr_ok;
}

int memcache_connected(connection *c) {
  if (c->Tmp != nullptr) {
    int query_len = get_total_ready_bytes(c->Tmp);
    copy_through(&c->Out, c->Tmp, query_len);
  }

  return 0;
}

int memcache_client_execute(connection *c, int op) {
  mcc_data *D = MCC_DATA(c);
  int len, x = 0;
  char *ptr;

  tvkprintf(php_connections, 3, "proxy_mc_client: op=%d, key_len=%d, arg#=%d, response_len=%d\n", op, D->key_len, D->arg_num, D->response_len);

  if (op == mcrt_empty) {
    return SKIP_ALL_BYTES;
  }

  conn_query *cur_query = nullptr;
  if (c->first_query == (conn_query *)c) {
    if (op != mcrt_VERSION) {
      kprintf("response received for empty query list? op=%d\n", op);
      dump_connection_buffers(c);
      D->response_flags |= 16;
      return SKIP_ALL_BYTES;
    }
  } else {
    cur_query = c->first_query;
  }

  c->last_response_time = precise_now;

  int query_len;
  data_reader_t *reader;
  switch (op) {
    case mcrt_empty:
      return SKIP_ALL_BYTES;

    case mcrt_VALUE: {
      constexpr size_t MAX_VALUE_LEN = (1u << 24u);
      constexpr size_t MAX_KEY_LEN = 1000;
      if (D->key_len > 0 && D->key_len <= MAX_KEY_LEN && D->arg_num == 2 && (unsigned)D->args[1] <= MAX_VALUE_LEN) {
        int needed_bytes = (int)(D->args[1] + D->response_len + 2 - c->In.total_bytes);
        if (needed_bytes > 0) {
          return needed_bytes;
        }
        nbit_advance(&c->Q, (int)D->args[1]);
        len = nbit_ready_bytes(&c->Q);
        assert(len > 0);
        ptr = reinterpret_cast<char *>(nbit_get_ptr(&c->Q));
      } else {
        kprintf("error at VALUE: op=%d, key_len=%d, arg_num=%d, value_len=%lld\n", op, D->key_len, D->arg_num, D->args[1]);

        D->response_flags |= 16;
        return SKIP_ALL_BYTES;
      }
      if (len == 1) {
        nbit_advance(&c->Q, 1);
      }
      if (ptr[0] != '\r' || (len > 1 ? ptr[1] : *((char *)nbit_get_ptr(&c->Q))) != '\n') {
        kprintf("missing cr/lf at VALUE: op=%d, key_len=%d, arg_num=%d, value_len=%lld\n", op, D->key_len, D->arg_num, D->args[1]);

        assert(0);

        D->response_flags |= 16;
        return SKIP_ALL_BYTES;
      }
      len = 2;

      tvkprintf(php_connections, 3, "mcc_value: op=%d, key_len=%d, flags=%lld, time=%lld, value_len=%lld\n", op, D->key_len, D->args[0], D->args[1],
                D->args[2]);

      query_len = (int)(D->response_len + D->args[1] + len);
      reader = create_data_reader(c, query_len);
      x = mc_query_value(c->first_query, reader);
      if (x) {
        fail_connection(c, -7);
        c->ready = cr_failed;
      }
      if (reader->readed) {
        return 0;
      }
      assert(advance_skip_read_ptr(&c->In, query_len) == query_len);
      return 0;
    }
    case mcrt_VERSION:
      c->unreliability >>= 1;
      tvkprintf(php_connections, 3, "mcc_got_version: op=%d, key_len=%d, unreliability=%d\n", op, D->key_len, c->unreliability);

      if (cur_query != nullptr) {
        query_len = D->response_len;
        reader = create_data_reader(c, query_len);
        x = mc_query_version(cur_query, reader);
        if (x) {
          fail_connection(c, -8);
          c->ready = cr_failed;
        }
        if (reader->readed) {
          return 0;
        }
      }
      return SKIP_ALL_BYTES;

    case mcrt_CLIENT_ERROR:
      kprintf("CLIENT_ERROR received from connection %d (%s)\n", c->fd, sockaddr_storage_to_string(&c->remote_endpoint));
      // client_errors_received++;
      /* fallthrough */
    case mcrt_ERROR:
      // errors_received++;
      /*if (verbosity > -2 && errors_received < 32) {
        dump_connection_buffers (c);
        if (c->first_query != (conn_query *) c && c->first_query->req_generation == c->first_query->requester->generation) {
          dump_connection_buffers (c->first_query->requester);
        }
      }*/

      fail_connection(c, -5);
      c->ready = cr_failed;
      // conn_query will be closed during connection closing
      return SKIP_ALL_BYTES;
    case mcrt_STORED:
    case mcrt_NOTSTORED:
      x = mc_query_xstored(c->first_query, op == mcrt_STORED);
      if (x) {
        fail_connection(c, -6);
        c->ready = cr_failed;
      }
      return SKIP_ALL_BYTES;
    case mcrt_SERVER_ERROR:
    case mcrt_NUMBER:
    case mcrt_DELETED:
    case mcrt_NOTFOUND:
      query_len = D->response_len;
      reader = create_data_reader(c, query_len);
      x = mc_query_other(c->first_query, reader);
      if (x) {
        fail_connection(c, -7);
        c->ready = cr_failed;
      }
      if (reader->readed) {
        return 0;
      }
      assert(advance_skip_read_ptr(&c->In, query_len) == query_len);
      return 0;
      break;

    case mcrt_END:
      mc_query_end(c->first_query);
      return SKIP_ALL_BYTES;

    default:
      assert("unknown state" && 0);
  }

  assert("unreachable position" && 0);
  return 0;
}

void php_worker_run_mc_query_packet(PhpWorker *worker, php_net_query_packet_t *query) {
  query_stats.desc = "MC";
  query_stats.query = query->data;

  php_script->query_readed();
  mc_ansgen_t *ansgen = mc_ansgen_packet_create();
  ansgen->func->set_query_type(ansgen, query->extra_type);

  auto *net_ansgen = (net_ansgen_t *)ansgen;
  int connection_id = query->connection_id;

  if (connection_id < 0 || connection_id >= MAX_TARGETS) {
    net_error(net_ansgen, (php_query_base_t *)query, "Invalid connection_id (1)");
    return;
  }

  conn_target_t *target = &Targets[connection_id];

  net_ansgen->func->set_desc(net_ansgen, qmem_pstr("[%s]", sockaddr_storage_to_string(&target->endpoint)));

  query_stats.port = inet_sockaddr_port(&target->endpoint);

  connection *conn = get_target_connection_force(target);
  if (conn == nullptr) {
    net_error(net_ansgen, (php_query_base_t *)query, "Failed to establish connection [probably reconnect timeout is not expired]");
    return;
  }

  if (conn->status != conn_connecting) {
    write_out(&conn->Out, query->data, query->data_len);
    MCC_FUNC(conn)->flush_query(conn);
  } else {
    if (conn->Tmp == nullptr) {
      conn->Tmp = alloc_head_buffer();
    }
    write_out(conn->Tmp, query->data, query->data_len);
  }

  double timeout = fix_timeout(query->timeout) + precise_now;
  conn_query *cq = create_pnet_query(worker->conn, conn, (net_ansgen_t *)ansgen, timeout);

  if (query->extra_type & PNETF_IMMEDIATE) {
    pnet_query_timeout(cq);
  } else if (worker->conn != nullptr) {
    worker->conn->status = conn_wait_net;
  }
}

memcache_client_functions memcache_client_outbound = [] {
  auto res = memcache_client_functions();
  res.execute = memcache_client_execute;
  res.check_ready = memcache_client_check_ready;
  res.connected = memcache_connected;

  res.flush_query = mcc_flush_query;
  res.mc_check_perm = mcc_default_check_perm;
  res.mc_init_crypto = mcc_init_crypto;
  res.mc_start_crypto = mcc_start_crypto;

  return res;
}();

conn_target_t memcache_ct = [] {
  auto res = conn_target_t();
  res.min_connections = 2;
  res.max_connections = 3;
  res.type = &ct_memcache_client;
  res.extra = &memcache_client_outbound;
  res.reconnect_timeout = 1;

  return res;
}();
