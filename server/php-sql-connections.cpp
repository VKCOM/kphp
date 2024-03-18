// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "server/php-sql-connections.h"

#include "net/net-connections.h"
#include "net/net-mysql-client.h"

#include "server/php-engine.h"
#include "server/php-queries.h"
#include "server/php-runner.h"
#include "server/php-worker.h"

#define RESPONSE_FAIL_TIMEOUT 30.0

void command_net_write_run_sql(command_t *base_command, void *data);

command_t command_net_write_sql_base = {.run = command_net_write_run_sql, .free = command_net_write_free};

static const char *mysql_db_name = "dbname_not_set";
static const char *mysql_user = "fake";
static const char *mysql_password = "fake";
static bool is_mysql_same_datacenter_check_disabled = false;

int proxy_client_execute(connection *c, int op);

int sqlp_authorized(connection *c);
int sqlp_becomes_ready(connection *c);
int sqlp_check_ready(connection *c);

bool set_mysql_db_name(const char *db_name) {
  mysql_db_name = db_name;
  return true;
}
bool set_mysql_user(const char *user) {
  mysql_user = user;
  return true;
}
bool set_mysql_password(const char *password) {
  mysql_password = password;
  return true;
}
void disable_mysql_same_datacenter_check() {
  is_mysql_same_datacenter_check_disabled = true;
}

mysql_client_functions db_client_outbound = [] {
  auto res = mysql_client_functions();
  res.execute = proxy_client_execute;
  res.sql_authorized = sqlp_authorized;
  res.sql_becomes_ready = sqlp_becomes_ready;
  res.check_ready = sqlp_check_ready;
  res.sql_flush_packet = sqlc_flush_packet;
  res.sql_check_perm = sqlc_default_check_perm;
  res.sql_init_crypto = sqlc_init_crypto;
  // when connecting to MySQL from PHP and KPHP, login credentials do not matter due to rpc proxy, only db name matters
  res.sql_get_database = [](connection *) { return mysql_db_name; };
  res.sql_get_user = [](connection *) { return mysql_user; };
  res.sql_get_password = [](connection *) { return mysql_password; };
  res.is_mysql_same_datacenter_check_disabled = []() { return is_mysql_same_datacenter_check_disabled; };

  return res;
}();

conn_target_t db_ct = [] {
  auto res = conn_target_t();
  res.min_connections = 2;
  res.max_connections = 3;
  res.type = &ct_mysql_client;
  res.extra = &db_client_outbound;
  res.reconnect_timeout = 1;

  return res;
}();

void command_net_write_run_sql(command_t *base_command, void *data) {
  // fprintf (stderr, "command_net_write [ptr=%p]\n", base_command);
  auto *command = (command_net_write_t *)base_command;

  assert(command->data != nullptr);
  auto *d = (connection *)data;
  assert(d->status == conn_ready);

  /*
  {
    fprintf (stderr, "packet_len = %d\n", command->len);
    int s = command->len;
    fprintf (stderr, "got prefix [len = %d]\n", s);
    unsigned char *packet = command->data;
    if (s > 5) {
      int len = (packet[0]) + (packet[1] << 8) + (packet[2] << 16);
      int packet_number = packet[3];
      int command_type = packet[4];
      fprintf (stderr, "[len = %d], [num = %d], [type = %d], [%.*s]\n", len, packet_number, command_type, s - 5, packet + 5);
    }
  }
  */

  assert(write_out(&(d)->Out, command->data, command->len) == command->len);
  SQLC_FUNC(d)->sql_flush_packet(d, command->len - 4);

  flush_connection_output(d);
  d->last_query_sent_time = precise_now;
  d->status = conn_wait_answer;
  SQLC_DATA(d)->response_state = resp_first;

  free(command->data);
  command->data = nullptr;
  command->len = 0;
}

void php_worker_run_sql_query_packet(PhpWorker *worker, php_net_query_packet_t *query) {
  query_stats.desc = "SQL";
  query_stats.query = query->data;

  int connection_id = query->connection_id;
  php_script->query_readed();

  sql_ansgen_t *ansgen = sql_ansgen_packet_create();

  auto *net_ansgen = (net_ansgen_t *)ansgen;
  if (connection_id != sql_target_id) {
    net_error(net_ansgen, (php_query_base_t *)query, "Invalid connection_id (sql connection expected)");
    return;
  }

  if (connection_id < 0 || connection_id >= MAX_TARGETS) {
    net_error(net_ansgen, (php_query_base_t *)query, "Invalid connection_id (1)");
    return;
  }

  conn_target_t *target = &Targets[connection_id];

  net_ansgen->func->set_desc(net_ansgen, qmem_pstr("[%s]", sockaddr_storage_to_string(&target->endpoint)));

  connection *conn = get_target_connection(target, 0);

  double timeout = fix_timeout(query->timeout) + precise_now;
  if (conn != nullptr && conn->status == conn_ready) {
    write_out(&conn->Out, query->data, query->data_len);
    SQLC_FUNC(conn)->sql_flush_packet(conn, query->data_len - 4);
    flush_connection_output(conn);
    conn->last_query_sent_time = precise_now;
    conn->status = conn_wait_answer;
    SQLC_DATA(conn)->response_state = resp_first;

    ansgen->func->set_writer(ansgen, nullptr);
    ansgen->func->ready(ansgen, nullptr);

    create_pnet_query(worker->conn, conn, net_ansgen, timeout);
  } else {
    int new_conn_cnt = create_new_connections(target);
    if (new_conn_cnt <= 0 && get_target_connection(target, 1) == nullptr) {
      net_error(net_ansgen, (php_query_base_t *)query, "Failed to establish connection [probably reconnect timeout is not expired]");
      return;
    }

    ansgen->func->set_writer(ansgen, create_command_net_writer(query->data, query->data_len, &command_net_write_sql_base, -1));
    create_pnet_delayed_query(worker->conn, target, net_ansgen, timeout);
  }

  if (worker->conn != nullptr) {
    worker->conn->status = conn_wait_net;
  }
}

int sql_query_packet(conn_query *q, data_reader_t *reader) {
  auto *ansgen = (sql_ansgen_t *)q->extra;
  ansgen->func->packet(ansgen, reader);

  return pnet_query_check(q);
}

int sql_query_done(conn_query *q) {
  auto *ansgen = (sql_ansgen_t *)q->extra;
  ansgen->func->done(ansgen);

  return pnet_query_check(q);
}

int sqlp_becomes_ready(connection *c) {
  conn_query *q;

  while (c->target->first_query != (conn_query *)(c->target)) {
    q = c->target->first_query;
    //    fprintf (stderr, "processing delayed query %p for target %p initiated by %p (%d:%d<=%d)\n", q, c->target, q->requester, q->requester->fd,
    //    q->req_generation, q->requester->generation);
    if (q->requester != nullptr && q->requester->generation == q->req_generation) {
      q->requester->queries_ok++;
      // waiting_queries--;

      auto *net_ansgen = (net_ansgen_t *)q->extra;
      create_pnet_query(q->requester, c, net_ansgen, q->timer.wakeup_time);

      delete_conn_query(q);
      free(q);

      auto *ansgen = (sql_ansgen_t *)net_ansgen;
      ansgen->func->ready(ansgen, c);
      break;
    } else {
      // waiting_queries--;
      pnet_query_delete(q);
    }
  }
  return 0;
}

int sqlp_check_ready(connection *c) {
  if (c->status == conn_ready && c->In.total_bytes > 0) {
    kprintf("have %d bytes in outbound sql connection %d in state ready, closing connection\n", c->In.total_bytes, c->fd);
    c->status = conn_error;
    c->error = -3;
    fail_connection(c, -3);
    return c->ready = cr_failed;
  }
  if (c->status == conn_error) {
    c->error = -4;
    fail_connection(c, -4);
    return c->ready = cr_failed;
  }
  if (c->status == conn_wait_answer || c->status == conn_reading_answer) {
    if (!(c->flags & C_FAILED) && c->last_query_sent_time < precise_now - RESPONSE_FAIL_TIMEOUT - c->last_query_time
        && c->last_response_time < precise_now - RESPONSE_FAIL_TIMEOUT - c->last_query_time && !(SQLC_DATA(c)->extra_flags & 1)) {
      tvkprintf(php_connections, 1, "failing outbound connection %d, status=%d, response_status=%d, last_response=%.6f, last_query=%.6f, now=%.6f\n", c->fd,
                c->status, SQLC_DATA(c)->response_state, c->last_response_time, c->last_query_sent_time, precise_now);
      c->error = -5;
      fail_connection(c, -5);
      return c->ready = cr_failed;
    }
  }
  return c->ready =
           (c->status == conn_ready ? cr_ok : (SQLC_DATA(c)->auth_state == sql_auth_ok ? cr_stopped : cr_busy)); /* was cr_busy instead of cr_stopped */
}

int sqlp_authorized(connection *c) {
  nbw_iterator_t it;
  unsigned temp = 0x00000000;
  int len = 0;
  char ptype = 2;

  const auto *sql_database = SQLC_FUNC(c)->sql_get_database(c);
  if (!sql_database || !*sql_database) {
    SQLC_DATA(c)->auth_state = sql_auth_ok;
    c->status = conn_ready;
    SQLC_DATA(c)->packet_state = 0;
    tvkprintf(php_connections, 1, "outcoming initdb successful\n");
    SQLC_FUNC(c)->sql_becomes_ready(c);
    return 0;
  }

  nbit_setw(&it, &c->Out);
  write_out(&c->Out, &temp, 4);

  len += write_out(&c->Out, &ptype, 1);
  if (sql_database && *sql_database) {
    len += write_out(&c->Out, sql_database, (int)strlen(sql_database));
  }

  nbit_write_out(&it, &len, 3);

  SQLC_FUNC(c)->sql_flush_packet(c, len);

  SQLC_DATA(c)->auth_state = sql_auth_initdb;
  return 0;
}

int proxy_client_execute(connection *c, int op) {
  sqlc_data *D = SQLC_DATA(c);
  static char buffer[8];
  int b_len, field_cnt = -1;
  nb_iterator_t it;

  nbit_set(&it, &c->In);
  b_len = nbit_read_in(&it, buffer, 8);

  if (b_len >= 5) {
    field_cnt = buffer[4] & 0xff;
  }
  tvkprintf(php_connections, 3, "proxy client execute: op=%d, packet_len=%d, response_state=%d, field_num=%d\n", op, D->packet_len, D->response_state,
            field_cnt);

  if (c->first_query == (conn_query *)c) {
    kprintf("response received for empty query list? op=%d\n", op);
    return SKIP_ALL_BYTES;
  }

  c->last_response_time = precise_now;

  int query_len = D->packet_len + 4;
  data_reader_t *reader = create_data_reader(c, query_len);

  int x;
  switch (D->response_state) {
    case resp_first:
      // forward_response (c, D->packet_len, SQLC_DATA(c)->extra_flags & 1);
      x = sql_query_packet(c->first_query, reader);

      if (!reader->readed) {
        assert(advance_skip_read_ptr(&c->In, query_len) == query_len);
      }

      if (x) {
        fail_connection(c, -6);
        c->ready = cr_failed;
        return 0;
      }

      if (field_cnt == 0 && (SQLC_DATA(c)->extra_flags & 1)) {
        dl_unreachable("looks like unused code");
        SQLC_DATA(c)->extra_flags |= 2;
        if (c->first_query->requester->generation != c->first_query->req_generation) {
          tvkprintf(php_connections, 3, "outbound connection %d: nowhere to forward replication stream, closing\n", c->fd);
          c->status = conn_error;
        }
      } else if (field_cnt == 0 || field_cnt == 0xff) {
        D->response_state = resp_done;
      } else if (field_cnt < 0 || field_cnt >= 0xfe) {
        c->status = conn_error; // protocol error
      } else {
        D->response_state = resp_reading_fields;
      }
      break;
    case resp_reading_fields:
      // forward_response (c, D->packet_len, 0);
      x = sql_query_packet(c->first_query, reader);
      if (!reader->readed) {
        assert(advance_skip_read_ptr(&c->In, query_len) == query_len);
      }

      if (x) {
        fail_connection(c, -7);
        c->ready = cr_failed;
        return 0;
      }

      if (field_cnt == 0xfe) {
        D->response_state = resp_reading_rows;
      }
      break;
    case resp_reading_rows:
      // forward_response (c, D->packet_len, 0);
      x = sql_query_packet(c->first_query, reader);
      if (!reader->readed) {
        assert(advance_skip_read_ptr(&c->In, query_len) == query_len);
      }
      if (x) {
        fail_connection(c, -8);
        c->ready = cr_failed;
        return 0;
      }
      if (field_cnt == 0xfe) {
        D->response_state = resp_done;
      }
      break;
    case resp_done:
      kprintf("unexpected packet from server!\n");
      assert(0);
  }

  if (D->response_state == resp_done) {
    //    query_complete (c, 1);
    x = sql_query_done(c->first_query);
    if (x) {
      fail_connection(c, -9);
      c->ready = cr_failed;
      return 0;
    }

    // active_queries--;
    c->unreliability >>= 1;
    c->status = conn_ready;
    c->last_query_time = precise_now - c->last_query_sent_time;
    // SQLC_DATA(c)->extra_flags &= -2;
    sqlp_becomes_ready(c);
  }

  return 0;
}
