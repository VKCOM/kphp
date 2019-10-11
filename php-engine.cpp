#include "PHP/php-engine.h"

#include <cassert>
#include <cstdlib>
#include <cstring>
#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <poll.h>
#include <sys/mman.h>
#include <sys/prctl.h>
#include <sys/socket.h>
#include <unistd.h>

#include "auto/TL/constants/common.h"
#include "auto/TL/constants/kphp.h"
#include "common/allocators/zmalloc.h"
#include "common/crc32c.h"
#include "common/cycleclock.h"
#include "common/kprintf.h"
#include "common/options.h"
#include "common/precise-time.h"
#include "common/resolver.h"
#include "common/rpc-error-codes.h"
#include "common/server/limits.h"
#include "common/server/relogin.h"
#include "common/server/signals.h"
#include "common/server/sockets.h"
#include "common/tl/parse.h"
#include "db-proxy/passwd.h"
#include "drinkless/dl-utils-lite.h"
#include "net/net-buffers.h"
#include "net/net-connections.h"
#include "net/net-crypto-aes.h"
#include "net/net-http-server.h"
#include "net/net-memcache-client.h"
#include "net/net-memcache-server.h"
#include "net/net-mysql-client.h"
#include "net/net-sockaddr-storage.h"
#include "net/net-socket.h"
#include "net/net-tcp-connections.h"
#include "net/net-tcp-rpc-client.h"
#include "net/net-tcp-rpc-server.h"

#include "PHP/php-engine-vars.h"
#include "PHP/php-lease.h"
#include "PHP/php-master.h"
#include "PHP/php-queries.h"
#include "PHP/php-runner.h"
#include "PHP/php-worker.h"
#include "runtime/interface.h"

static void turn_sigterm_on();

#define MAX_TIMEOUT 30

#define RPC_DEFAULT_PING_INTERVAL 10.0
double rpc_ping_interval = RPC_DEFAULT_PING_INTERVAL;
#define RPC_STOP_INTERVAL (2 * rpc_ping_interval)
#define RPC_FAIL_INTERVAL (3 * rpc_ping_interval)

#define RPC_CONNECT_TIMEOUT 3

/***
  DB-client
 ***/
#define RESPONSE_FAIL_TIMEOUT 30.0

int proxy_client_execute(connection *c, int op);

int sqlp_authorized(connection *c);
int sqlp_becomes_ready(connection *c);
int sqlp_check_ready(connection *c);

mysql_client_functions db_client_outbound = [] {
  auto res = mysql_client_functions();
  res.execute = proxy_client_execute;
  res.sql_authorized = sqlp_authorized;
  res.sql_becomes_ready = sqlp_becomes_ready;
  res.check_ready = sqlp_check_ready;
  res.sql_flush_packet = sqlc_flush_packet;
  res.sql_check_perm = sqlc_default_check_perm;
  res.sql_init_crypto = sqlc_init_crypto;

  return res;
}();


long long cur_request_id = -1;
/***
  MC-client
 ***/

int memcache_client_execute(connection *c, int op);
int memcache_client_check_ready(connection *c);
int memcache_connected(connection *c);

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

/***
  RPC-client
 ***/
int php_rpcc_execute(connection *c, int op, raw_message *raw);
int rpcc_send_query(connection *c);
int rpcc_check_ready(connection *c);
void send_rpc_query(connection *c, int op, long long id, int *q, int qsize);

tcp_rpc_client_functions tcp_rpc_client_outbound = [] {
  auto res = tcp_rpc_client_functions();
  res.execute = php_rpcc_execute; //replaced
  res.check_ready = rpcc_check_ready; //replaced
  res.flush_packet = tcp_rpcc_flush_packet_later;
  res.rpc_check_perm = tcp_rpcc_default_check_perm;
  res.rpc_init_crypto = tcp_rpcc_init_crypto;
  res.rpc_start_crypto = tcp_rpcc_start_crypto;;
  res.rpc_ready = rpcc_send_query; //replaced

  return res;
}();

/***
  OUTBOUND CONNECTIONS
 ***/

static int db_port = 3306;

conn_target_t default_ct = [] {
  auto res = conn_target_t();
  res.min_connections = 2;
  res.max_connections = 3;
  res.type = &ct_memcache_client;
  res.extra = &memcache_client_outbound;
  res.reconnect_timeout = 1;

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

conn_type_t ct_tcp_rpc_client_read_all = [] {
  auto res = get_default_tcp_rpc_client_conn_type();
  res.reader = tcp_server_reader_till_end;
  return res;
}();

conn_target_t rpc_ct = [] {
  auto res = conn_target_t();
  res.min_connections = 2;
  res.max_connections = 3;
  res.type = &ct_tcp_rpc_client_read_all;
  res.extra = (void *)&tcp_rpc_client_outbound;
  res.reconnect_timeout = 1;

  return res;
}();


connection *get_target_connection(conn_target_t *S, int force_flag) {
  connection *c, *d = nullptr;
  int u = 10000;
  if (!S) {
    return nullptr;
  }
  for (c = S->first_conn; c != (connection *)S; c = c->next) {
    int r = S->type->check_ready(c);
    if (r == cr_ok || (force_flag && r == cr_notyet)) {
      return c;
    } else if (r == cr_stopped && c->unreliability < u) {
      u = c->unreliability;
      d = c;
    }
  }
  return d;
}

connection *get_target_connection_force(conn_target_t *S) {
  connection *res = get_target_connection(S, 0);

  if (res == nullptr) {
    create_new_connections(S);
    res = get_target_connection(S, 1);
  }

  return res;
}

int get_target_impl(conn_target_t *ct) {
  //TODO: fix ref_cnt overflow
  conn_target_t *res = create_target(ct, nullptr);
  int res_id = (int)(res - Targets);
  return res_id;
}

int get_target_by_pid(int ip, int port, conn_target_t *ct) {
  ct->endpoint = make_inet_sockaddr_storage(ip, port);

  return get_target_impl(ct);
}

int get_target(const char *host, int port, conn_target_t *ct) {
  if (!(0 <= port && port < 0x10000)) {
    vkprintf (0, "bad port %d\n", port);
    return -1;
  }

  hostent *h;
  if (!(h = kdb_gethostbyname(host)) || h->h_addrtype != AF_INET || h->h_length != 4 || !h->h_addr_list || !h->h_addr) {
    vkprintf (0, "can't resolve host\n");
    return -1;
  }

  ct->endpoint = make_inet_sockaddr_storage(ntohl(*(uint32_t *)h->h_addr), port);

  return get_target_impl(ct);
}

double fix_timeout(double timeout) {
  if (timeout < 0) {
    return 0;
  }
  if (timeout > MAX_TIMEOUT) {
    return MAX_TIMEOUT;
  }
  return timeout;
}

/***
  HTTP INTERFACE
 ***/
void http_return(connection *c, const char *str, int len);

int delete_pending_query(conn_query *q) {
  vkprintf (1, "delete_pending_query(%p,%p)\n", q, q->requester);

  delete_conn_query(q);
  zfree(q, sizeof(*q));
  return 0;
}

conn_query_functions pending_cq_func = [] {
  auto res = conn_query_functions();
  res.magic = (int)CQUERY_FUNC_MAGIC;
  res.title = "pending-query";
  res.wakeup = delete_pending_query;
  res.close = delete_pending_query;
  res.complete = delete_pending_query;

  return res;
}();

/** delayed httq queries queue **/
connection pending_http_queue;
int php_worker_run_flag;

/** dummy request queue **/
connection dummy_request_queue;

void on_net_event(int event_status);

/** net write command **/
/** do write delayed write to connection **/


struct command_net_write_t {
  command_t base;

  unsigned char *data;
  int len;
  long long extra;
};

void command_net_write_run_sql(command_t *base_command, void *data) {
  //fprintf (stderr, "command_net_write [ptr=%p]\n", base_command);
  auto command = (command_net_write_t *)base_command;

  assert (command->data != nullptr);
  auto d = (connection *)data;
  assert (d->status == conn_ready);

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

  assert (write_out(&(d)->Out, command->data, command->len) == command->len);
  SQLC_FUNC (d)->sql_flush_packet(d, command->len - 4);

  flush_connection_output(d);
  d->last_query_sent_time = precise_now;
  d->status = conn_wait_answer;
  SQLC_DATA(d)->response_state = resp_first;


  free(command->data);
  command->data = nullptr;
  command->len = 0;
}

void command_net_write_run_rpc(command_t *base_command, void *data) {
  auto *command = (command_net_write_t *)base_command;
//  fprintf (stderr, "command_net_write [ptr=%p] [len = %d] [data = %p]\n", base_command, command->len, data);

  slot_id_t slot_id = command->extra;
  assert (command->data != nullptr);
  if (data == nullptr) { //send to /dev/null
    vkprintf (3, "failed to send rpc request %d\n", slot_id);
    on_net_event(create_rpc_error_event(slot_id, TL_ERROR_NO_CONNECTIONS, "Failed to send query, timeout expired", nullptr));
  } else {
    auto d = (connection *)data;
    //assert (d->status == conn_ready);
    send_rpc_query(d, TL_RPC_INVOKE_REQ, slot_id, (int *)command->data, command->len);
    d->last_query_sent_time = precise_now;
  }
}


void command_net_write_free(command_t *base_command) {
  auto *command = (command_net_write_t *)base_command;

  if (command->data != nullptr) {
    free(command->data);
    command->data = nullptr;
    command->len = 0;
  }
  free(command);
}

command_t command_net_write_sql_base = {
  .run = command_net_write_run_sql,
  .free = command_net_write_free
};

command_t command_net_write_rpc_base = {
  .run = command_net_write_run_rpc,
  .free = command_net_write_free
};


command_t *create_command_net_writer(const char *data, int data_len, command_t *base, long long extra) {
  auto command = reinterpret_cast<command_net_write_t *>(malloc(sizeof(command_net_write_t)));
  command->base.run = base->run;
  command->base.free = base->free;

  command->data = reinterpret_cast<unsigned char *>(malloc((size_t)data_len));
  memcpy(command->data, data, (size_t)data_len);
  command->len = data_len;
  command->extra = extra;

  return reinterpret_cast<command_t *>(command);
}


/** php-script **/

#define run_once_count 1
int queries_to_recreate_script = 100;

void *php_script;

php_worker *active_worker = nullptr;

php_worker *php_worker_create(php_worker_mode_t mode, connection *c, http_query_data *http_data, rpc_query_data *rpc_data, double timeout, long long req_id) {
  auto worker = reinterpret_cast<php_worker *>(dl_malloc(sizeof(php_worker)));

  worker->data = php_query_data_create(http_data, rpc_data);
  worker->conn = c;
  assert (c != nullptr);

  worker->state = phpq_try_start;
  worker->mode = mode;

  worker->init_time = precise_now;
  worker->finish_time = precise_now + timeout;

  worker->paused = 0;
  worker->terminate_flag = 0;
  worker->error_message = "no error";

  worker->waiting = 0;
  worker->wakeup_flag = 0;
  worker->wakeup_time = 0;

  worker->req_id = req_id;

  if (worker->conn->target) {
    worker->target_fd = worker->conn->target - Targets;
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

  dl_free(worker, sizeof(php_worker));
}

int has_pending_scripts() {
  return php_worker_run_flag || pending_http_queue.first_query != (conn_query *)&pending_http_queue;
}

/** trying to start query **/
void php_worker_try_start(php_worker *worker) {

  if (worker->terminate_flag) {
    worker->state = phpq_finish;
    return;
  }

  if (php_worker_run_flag) { // put connection into pending_http_query
    vkprintf (2, "php script [req_id = %016llx] is waiting\n", worker->req_id);

    auto pending_q = reinterpret_cast<conn_query *>(zmalloc(sizeof(conn_query)));

    pending_q->custom_type = 0;
    pending_q->outbound = (connection *)&pending_http_queue;
    assert (worker->conn != nullptr);
    pending_q->requester = worker->conn;

    pending_q->cq_type = &pending_cq_func;
    pending_q->timer.wakeup_time = worker->finish_time;

    insert_conn_query(pending_q);

    worker->conn->status = conn_wait_net;

    worker->paused = 1;
    return;
  }

  php_worker_run_flag = 1;
  worker->state = phpq_init_script;
}

void php_worker_init_script(php_worker *worker) {
  double timeout = worker->finish_time - precise_now - 0.01;
  if (worker->terminate_flag || timeout < 0.2) {
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

  script_t *script = get_script("#0");
  dl_assert (script != nullptr, "failed to get script");
  if (php_script == nullptr) {
    php_script = php_script_create((size_t)max_memory, (size_t)(8 << 20));
  }
  php_script_init(php_script, script, worker->data);
  php_script_set_timeout(timeout);
  worker->state = phpq_run;
}

void php_worker_terminate(php_worker *worker, int flag, const char *error_message) {
  worker->terminate_flag = 1;
  worker->error_message = error_message;
  if (flag) {
    vkprintf (0, "php_worker_terminate\n");
    worker->conn = nullptr;
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
      res.connection_id = get_target(query->host, query->port, &default_ct);
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

conn_query *create_pnet_query(connection *http_conn, connection *mc_conn, net_ansgen_t *gen, double finish_time);
void create_pnet_delayed_query(connection *http_conn, conn_target_t *t, net_ansgen_t *gen, double finish_time);
int pnet_query_timeout(conn_query *q);
void create_delayed_send_query(conn_target_t *t, command_t *command,
                               double finish_time);

void net_error(net_ansgen_t *ansgen, php_query_base_t *query, const char *err) {
  ansgen->func->error(ansgen, err);
  query->ans = ansgen->ans;
  ansgen->func->free(ansgen);
  php_script_query_answered(php_script);
}

void php_worker_run_mc_query_packet(php_worker *worker, php_net_query_packet_t *query) {
  query_stats.desc = "MC";
  query_stats.query = query->data;

  php_script_query_readed(php_script);
  mc_ansgen_t *ansgen = mc_ansgen_packet_create();
  ansgen->func->set_query_type(ansgen, query->extra_type);

  auto net_ansgen = (net_ansgen_t *)ansgen;
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
    MCC_FUNC (conn)->flush_query(conn);
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

void php_worker_run_sql_query_packet(php_worker *worker, php_net_query_packet_t *query) {
  query_stats.desc = "SQL";
  query_stats.query = query->data;

  int connection_id = query->connection_id;
  php_script_query_readed(php_script);

  sql_ansgen_t *ansgen = sql_ansgen_packet_create();

  auto net_ansgen = (net_ansgen_t *)ansgen;
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
    SQLC_FUNC (conn)->sql_flush_packet(conn, query->data_len - 4);
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

void prepare_rpc_query_raw(int packet_id, int *q, int qsize, unsigned (*crc32_partial_custom)(const void *q, long len, unsigned crc32_complement)) {
  static_assert(sizeof(int) == 4, "");
  q[0] = qsize;
  assert ((qsize & 3) == 0);
  int qlen = qsize >> 2;
  assert (qlen >= 5);

  q[1] = packet_id;
  q[qlen - 1] = (int)~crc32_partial_custom(q, q[0] - 4, 0xffffffff);
}

void send_rpc_query(connection *c, int op, long long id, int *q, int qsize) {
  q[2] = op;
  if (id != -1) {
    *(long long *)(q + 3) = id;
  }

  vkprintf (4, "send_rpc_query: [len = %d] [op = %08x] [rpc_id = <%lld>]\n", q[0], op, id);
  tcp_rpc_conn_send_data(c, qsize - 3 * sizeof(int), q + 2);

  TCP_RPCS_FUNC(c)->flush_packet(c);
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

      r = recv(c->fd, buf + read, max_len - read, 0);
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
    php_worker_terminate(worker, 1, "error during loading big post data"); //TODO we need to close connection. Do we need to pass 1 as second parameter?
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

void on_net_event(int event_status) {
  if (event_status == 0) {
    return;
  }
  assert (active_worker != nullptr);
  if (event_status < 0) {
    php_worker_terminate(active_worker, 0, "memory limit(?)");
    php_worker_wakeup(active_worker);
    return;
  }
  if (active_worker->waiting) {
    php_worker_wakeup(active_worker);
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

extern int rpc_stored;

void rpc_error(php_worker *worker, int code, const char *str) {
  connection *c = worker->conn;
  //fprintf (stderr, "RPC ERROR %s\n", str);
  static int q[10000];
  q[2] = TL_RPC_REQ_ERROR;
  *(long long *)(q + 3) = worker->req_id;
  q[5] = code;
  //TODO: write str

  char *buf = (char *)(q + 6);
  int all_len = sizeof(q[2]) + sizeof(long long) + sizeof(q[5]);
  int sn = (int)strlen(str);

  if (sn > 5000) {
    sn = 5000;
  }

  if (sn < 254) {
    *buf++ = (char)(sn);
    all_len += 1;
  } else if (sn < (1 << 24)) {
    *buf++ = (char)(254);
    *buf++ = (char)(sn & 255);
    *buf++ = (char)((sn >> 8) & 255);
    *buf++ = (char)((sn >> 16) & 255);
    all_len += 4;
  } else {
    assert ("TODO: store too big string" && 0);
  }

  memcpy(buf, str, (size_t)sn);
  buf += sn;
  all_len += sn;
  while (all_len % 4 != 0) {
    *buf++ = 0;
    all_len++;
  }

  tcp_rpc_conn_send_data(c, all_len, q + 2);

  TCP_RPCS_FUNC(c)->flush_packet(c);
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
        rpc_error(worker, -505, "Nothing stored");
      }
    } else if (worker->mode == once_worker) {
      assert (write(1, res->body, (size_t)res->body_len) == res->body_len);
      run_once_return_code = res->exit_code;
    }
  }
}

void php_worker_run_net_queue(php_worker *worker __attribute__((unused))) {
  net_query_t *query;
  while ((query = pop_net_query()) != nullptr) {
    //no other types of query are currenly supported
    php_worker_run_rpc_send_query(query);
    free_net_query(query);
  }
}

void php_worker_run(php_worker *worker) {
  int f = 1;
  while (f) {
    if (worker->terminate_flag) {
      php_script_terminate(php_script, worker->error_message, script_error_t::worker_terminate);
    }

//    fprintf (stderr, "state = %d, f = %d\n", php_script_get_state (php_script), f);
    switch (php_script_get_state(php_script)) {
      case run_state_t::ready: {
        running_server_status();
        if (worker->waiting) {
          f = 0;
          worker->paused = 1;
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
          worker->paused = 1;
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
        worker->paused = 1;
        wait_net_server_status();
        break;
      }
      case run_state_t::error: {
        vkprintf (2, "php script [req_id = %016llx]: ERROR (probably timeout)\n", worker->req_id);
        php_script_finish(php_script);

        if (worker->conn != nullptr) {
          if (worker->mode == http_worker) {
            http_return(worker->conn, "ERROR", 5);
          } else if (worker->mode == rpc_worker) {
            if (!rpc_stored) {
              rpc_error(worker, -504, php_script_get_error(php_script));
            }
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

double php_worker_main(php_worker *worker) {
  if (worker->finish_time < precise_now + 0.01) {
    php_worker_terminate(worker, 0, "timeout");
  }
  php_worker_on_wakeup(worker);

  worker->paused = 0;
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


int hts_php_wakeup(connection *c);
int hts_php_alarm(connection *c);

conn_type_t ct_php_engine_http_server = [] {
  auto res = conn_type_t();
  res.magic = CONN_FUNC_MAGIC;
  res.title = "http_server";
  res.accept = accept_new_connections;
  res.init_accepted = hts_init_accepted;
  res.run = server_read_write;
  res.reader = server_reader;
  res.writer = server_writer;
  res.parse_execute = hts_parse_execute;
  res.close = hts_close_connection;
  res.free_buffers = free_connection_buffers;
  res.init_outbound = server_failed;
  res.connected = server_failed;
  res.wakeup = hts_php_wakeup;
  res.alarm = hts_php_wakeup;

  return res;
}();

int hts_php_wakeup(connection *c) {
  if (c->status == conn_wait_net || c->status == conn_wait_aio) {
    c->status = conn_expect_query;
    HTS_FUNC(c)->ht_wakeup(c);
  }
  if (c->Out.total_bytes > 0) {
    c->flags |= C_WANTWR;
  }
  //c->generation = ++conn_generation;
  //c->pending_queries = 0;
  return 0;
}

int hts_php_alarm(connection *c) {
  HTS_FUNC(c)->ht_alarm(c);
  if (c->Out.total_bytes > 0) {
    c->flags |= C_WANTWR;
  }
  /*c->generation = ++conn_generation;*/
  /*c->pending_queries = 0;*/
  return 0;
}


int hts_func_wakeup(connection *c);
int hts_func_execute(connection *c, int op);
int hts_func_close(connection *c, int who);

http_server_functions http_methods = [] {
  auto res = http_server_functions();
  res.execute = hts_func_execute;
  res.ht_wakeup = hts_func_wakeup;
  res.ht_alarm = hts_func_wakeup;
  res.ht_close = hts_func_close;

  return res;
}();

static char *qPost, *qGet, *qUri, *qHeaders;
static int qPostLen, qGetLen, qUriLen, qHeadersLen;

static char no_cache_headers[] =
  "Pragma: no-cache\r\n"
  "Cache-Control: no-store\r\n";

#define HTTP_RESULT_SIZE  (4 << 20)

//static char http_body_buffer[HTTP_RESULT_SIZE], *http_w;
//#define http_w_end  (http_body_buffer + HTTP_RESULT_SIZE - 16384)
//void http_return (connection *c, const char *str, int len);

void http_return(connection *c, const char *str, int len) {
  if (len < 0) {
    len = (int)strlen(str);
  }
  write_basic_http_header(c, 500, 0, len, no_cache_headers, "text/plain; charset=UTF-8");
  write_out(&c->Out, str, len);
}

#define MAX_POST_SIZE (1 << 18)

void hts_my_func_finish(connection *c __attribute__((unused))) {
/*  c->status = conn_expect_query;
  clear_connection_timeout (c);

  if (!(D->query_flags & QF_KEEPALIVE)) {
    c->status = conn_write_close;
    c->parse_state = -1;
  }*/
}

int hts_stopped = 0;

void hts_stop() {
  if (hts_stopped) {
    return;
  }
  if (http_sfd != -1) {
    epoll_close(http_sfd);
    close(http_sfd);
    http_sfd = -1;
  }
  sigterm_time = get_utime_monotonic() + SIGTERM_WAIT_TIMEOUT;
  hts_stopped = 1;
}

void hts_at_query_end(connection *c, int check_keep_alive) {
  hts_data *D = HTS_DATA (c);

  clear_connection_timeout(c);
  c->generation = ++conn_generation;
  c->pending_queries = 0;
  D->extra = nullptr;
  if (check_keep_alive && !(D->query_flags & QF_KEEPALIVE)) {
    c->status = conn_write_close;
    c->parse_state = -1;
  } else {
    c->flags |= C_REPARSE;
  }
  assert (c->status != conn_wait_net);
}

int do_hts_func_wakeup(connection *c, int flag) {
  hts_data *D = HTS_DATA(c);

  assert (c->status == conn_expect_query || c->status == conn_wait_net);
  c->status = conn_expect_query;

  auto worker = reinterpret_cast<php_worker *>(D->extra);
  double timeout = php_worker_main(worker);
  if (timeout == 0) {
    hts_at_query_end(c, flag);
  } else {
    assert (timeout > 0);
    set_connection_timeout(c, timeout);
    assert (c->pending_queries >= 0 && c->status == conn_wait_net);
  }
  return 0;
}

int hts_func_execute(connection *c, int op) {
  hts_data *D = HTS_DATA(c);
  static char ReqHdr[MAX_HTTP_HEADER_SIZE];
  static char Post[MAX_POST_SIZE];

  if (sigterm_on && sigterm_time < precise_now) {
    return -501;
  }

  vkprintf (1, "in hts_execute: connection #%d, op=%d, header_size=%d, data_size=%d, http_version=%d\n",
            c->fd, op, D->header_size, D->data_size, D->http_ver);

  if (!vk::any_of_equal(D->query_type, htqt_get, htqt_post, htqt_head)) {
    D->query_flags &= ~QF_KEEPALIVE;
    return -501;
  }

  if (D->data_size > 0) {
    int have_bytes = get_total_ready_bytes(&c->In);
    if (have_bytes < D->data_size + D->header_size && D->data_size < MAX_POST_SIZE) {
      vkprintf (1, "-- need %d more bytes, waiting\n", D->data_size + D->header_size - have_bytes);
      return D->data_size + D->header_size - have_bytes;
    }
  }

  assert (D->header_size <= MAX_HTTP_HEADER_SIZE);
  assert (read_in(&c->In, &ReqHdr, D->header_size) == D->header_size);

  qHeaders = ReqHdr + D->first_line_size;
  qHeadersLen = D->header_size - D->first_line_size;
  assert (D->first_line_size > 0 && D->first_line_size <= D->header_size);

  vkprintf (1, "===============\n%.*s\n==============\n", D->header_size, ReqHdr);
  vkprintf (1, "%d,%d,%d,%d\n", D->host_offset, D->host_size, D->uri_offset, D->uri_size);

  vkprintf (1, "hostname: '%.*s'\n", D->host_size, ReqHdr + D->host_offset);
  vkprintf (1, "URI: '%.*s'\n", D->uri_size, ReqHdr + D->uri_offset);

//  D->query_flags &= ~QF_KEEPALIVE;

  if (0 < D->data_size && D->data_size < MAX_POST_SIZE) {
    assert (read_in(&c->In, Post, D->data_size) == D->data_size);
    Post[D->data_size] = 0;
    vkprintf (1, "have %d POST bytes: `%.80s`\n", D->data_size, Post);
    qPost = Post;
    qPostLen = D->data_size;
  } else {
    qPost = nullptr;
    if (D->data_size > 0) {
      qPostLen = D->data_size;
    } else {
      qPostLen = 0;
    }
  }

  qUri = ReqHdr + D->uri_offset;
  qUriLen = D->uri_size;

  auto get_qm_ptr = reinterpret_cast<char *>(memchr(qUri, '?', (size_t)qUriLen));
  if (get_qm_ptr) {
    qGet = get_qm_ptr + 1;
    qGetLen = (int)(qUri + qUriLen - qGet);
    qUriLen = (int)(get_qm_ptr - qUri);
  } else {
    qGet = nullptr;
    qGetLen = 0;
  }

  if (qUriLen >= 200) {
    return -418;
  }

  vkprintf (1, "OK, lets do something\n");

  const char *query_type_str = nullptr;
  switch (D->query_type) {
    case htqt_get: query_type_str = "GET"; break;
    case htqt_post: query_type_str = "POST"; break;
    case htqt_head: query_type_str = "HEAD"; break;
    default: assert(0);
  }

  /** save query here **/
  http_query_data *http_data = http_query_data_create(qUri, qUriLen, qGet, qGetLen, qHeaders, qHeadersLen, qPost,
                                                      qPostLen, query_type_str, D->query_flags & QF_KEEPALIVE,
                                                      inet_sockaddr_address(&c->remote_endpoint),
                                                      inet_sockaddr_port(&c->remote_endpoint));

  static long long http_script_req_id = 0;
  php_worker *worker = php_worker_create(http_worker, c, http_data, nullptr, script_timeout, ++http_script_req_id);
  D->extra = worker;

  set_connection_timeout(c, script_timeout);
  c->status = conn_wait_net;
  return do_hts_func_wakeup(c, 0);
}

int hts_func_wakeup(connection *c) {
  return do_hts_func_wakeup(c, 1);
}

int hts_func_close(connection *c, int who __attribute__((unused))) {
  hts_data *D = HTS_DATA(c);

  auto worker = reinterpret_cast<php_worker *>(D->extra);
  if (worker != nullptr) {
    php_worker_terminate(worker, 1, "http connection close");
    double timeout = php_worker_main(worker);
    D->extra = nullptr;
    assert ("worker is unfinished after closing connection" && timeout == 0);
  }
  return 0;
}

/***
  RPC INTERFACE
 ***/
int rpcs_php_wakeup(connection *c);

int rpcc_php_wakeup(connection *c);

conn_type_t ct_php_engine_rpc_server = [] {
  auto res = conn_type_t();
  res.flags = C_RAWMSG;
  res.magic = CONN_FUNC_MAGIC;
  res.title = "rpc_server";
  res.accept = accept_new_connections;
  res.init_accepted = tcp_rpcs_init_accepted;
  res.run = server_read_write;
  res.reader = tcp_server_reader;
  res.writer = tcp_server_writer;
  res.parse_execute = tcp_rpcs_parse_execute;
  res.close = tcp_rpcs_close_connection;
  res.free_buffers = tcp_free_connection_buffers;
  res.init_outbound = server_failed;
  res.connected = server_failed;
  res.wakeup = rpcs_php_wakeup; //replaced
  res.alarm = rpcs_php_wakeup; //replaced
  res.crypto_init = aes_crypto_init;
  res.crypto_free = aes_crypto_free;
  res.crypto_encrypt_output = tcp_aes_crypto_encrypt_output;
  res.crypto_decrypt_input = tcp_aes_crypto_decrypt_input;
  res.crypto_needed_output_bytes = tcp_aes_crypto_needed_output_bytes;

  return res;
}();

conn_type_t ct_php_rpc_client = [] {
  auto res = conn_type_t();
  res.flags = C_RAWMSG;
  res.magic = CONN_FUNC_MAGIC;
  res.title = "rpc_client";
  res.accept = server_failed;
  res.init_accepted = server_failed;
  res.run = server_read_write;
  res.reader = tcp_server_reader;
  res.writer = tcp_server_writer;
  res.parse_execute = tcp_rpcc_parse_execute;
  res.close = tcp_rpcc_close_connection;
  res.free_buffers = tcp_free_connection_buffers;
  res.init_outbound = tcp_rpcc_init_outbound;
  res.connected = tcp_rpcc_connected;
  res.wakeup = rpcc_php_wakeup; // replaced
  res.alarm = rpcc_php_wakeup; // replaced
  res.check_ready = tcp_rpc_client_check_ready;
  res.crypto_init = aes_crypto_init;
  res.crypto_free = aes_crypto_free;
  res.crypto_encrypt_output = tcp_aes_crypto_encrypt_output;
  res.crypto_decrypt_input = tcp_aes_crypto_decrypt_input;
  res.crypto_needed_output_bytes = tcp_aes_crypto_needed_output_bytes;

  return res;
}();

int rpcs_php_wakeup(connection *c) {
  if (c->status == conn_wait_net) {
    c->status = conn_expect_query;
    TCP_RPCS_FUNC(c)->rpc_wakeup(c);
  }
  if (c->out_p.total_bytes > 0) {
    c->flags |= C_WANTWR;
  }
  //c->generation = ++conn_generation;
  //c->pending_queries = 0;
  return 0;
}

int rpcc_php_wakeup(connection *c) {
  if (c->status == conn_wait_net) {
    c->status = conn_expect_query;
    TCP_RPCC_FUNC(c)->rpc_wakeup(c);
  }
  if (c->out.total_bytes > 0) {
    c->flags |= C_WANTWR;
  }
  //c->generation = ++conn_generation;
  //c->pending_queries = 0;
  return 0;
}

int rpcx_execute(connection *c, int op, raw_message *raw);
int rpcx_func_wakeup(connection *c);
int rpcx_func_close(connection *c, int who);

int rpcc_func_ready(connection *c);

tcp_rpc_server_functions rpc_methods = [] {
  auto res = tcp_rpc_server_functions();
  res.execute = rpcx_execute; //replaced
  res.check_ready = server_check_ready;
  res.flush_packet = tcp_rpcs_flush_packet;
  res.rpc_check_perm = tcp_rpcs_default_check_perm;
  res.rpc_init_crypto = tcp_rpcs_init_crypto;
  res.rpc_wakeup = rpcx_func_wakeup; //replaced
  res.rpc_alarm = rpcx_func_wakeup; //replaced
  res.rpc_close = rpcx_func_close; //replaced

  return res;
}();

tcp_rpc_client_functions rpc_client_methods = [] {
  auto res = tcp_rpc_client_functions();
  res.execute = rpcx_execute; //replaced
  res.check_ready = server_check_ready; //replaced
  res.flush_packet = tcp_rpcc_flush_packet;
  res.rpc_check_perm = tcp_rpcc_default_check_perm;
  res.rpc_init_crypto = tcp_rpcc_init_crypto;
  res.rpc_start_crypto = tcp_rpcc_start_crypto;
  res.rpc_ready = rpcc_func_ready;
  res.rpc_wakeup = rpcx_func_wakeup; //replaced
  res.rpc_alarm = rpcx_func_wakeup; //replaced
  res.rpc_close = rpcx_func_close; //replaced

  return res;
}();

conn_target_t rpc_client_ct = [] {
  auto res = conn_target_t();
  res.min_connections = 1;
  res.max_connections = 1;
  res.type = &ct_php_rpc_client;
  res.extra = (void *)&rpc_client_methods;
  res.reconnect_timeout = 1;

  return res;
}();


int rpcc_func_ready(connection *c) {
  c->last_query_sent_time = precise_now;
  c->last_response_time = precise_now;

  int target_fd = c->target - Targets;
  if (target_fd == get_current_target() && !has_pending_scripts()) {
    lease_set_ready();
    run_rpc_lease();
  }
  return 0;
}


void rpcc_stop() {
  lease_on_stop();
  rpc_stopped = 1;
  sigterm_time = precise_now + SIGTERM_WAIT_TIMEOUT;
}

void rpcx_at_query_end(connection *c) {
  auto D = TCP_RPC_DATA(c);

  clear_connection_timeout(c);
  c->generation = ++conn_generation;
  c->pending_queries = 0;
  D->extra = nullptr;

  if (!has_pending_scripts()) {
    lease_set_ready();
    run_rpc_lease();
  }
  c->flags |= C_REPARSE;
  assert (c->status != conn_wait_net);
}

int rpcx_func_wakeup(connection *c) {
  auto D = TCP_RPC_DATA(c);

  assert (c->status == conn_expect_query || c->status == conn_wait_net);
  c->status = conn_expect_query;

  auto worker = reinterpret_cast<php_worker *>(D->extra);
  double timeout = php_worker_main(worker);
  if (timeout == 0) {
    rpcx_at_query_end(c);
  } else {
    assert (c->pending_queries >= 0 && c->status == conn_wait_net);
    assert (timeout > 0);
    set_connection_timeout(c, timeout);
  }
  return 0;
}

int rpcx_func_close(connection *c, int who __attribute__((unused))) {
  auto D = TCP_RPC_DATA(c);

  auto worker = reinterpret_cast<php_worker *>(D->extra);
  if (worker != nullptr) {
    php_worker_terminate(worker, 1, "rpc connection close");
    double timeout = php_worker_main(worker);
    D->extra = nullptr;
    assert ("worker is unfinished after closing connection" && timeout == 0);

    if (!has_pending_scripts()) {
      lease_set_ready();
      run_rpc_lease();
    }
  }

  return 0;
}


int rpcx_execute(connection *c, int op, raw_message *raw) {
  auto D = TCP_RPC_DATA(c);

  vkprintf (1, "rpcs_execute: fd=%d, op=%d, len=%d\n", c->fd, op, raw->total_bytes);

  int len = raw->total_bytes;

  if (sigterm_on && sigterm_time < precise_now) {
    return 0;
  }

  auto MAX_RPC_QUERY_LEN = 126214400;
  if (len < sizeof(long long) || MAX_RPC_QUERY_LEN < len) {
    return 0;
  }

  switch (op) {
    case TL_KPHP_STOP_LEASE:
      do_rpc_stop_lease();
      break;
    case TL_KPHP_START_LEASE:
    case TL_RPC_INVOKE_REQ: {

      tl_fetch_init_tcp_raw_message(raw, len);

      auto op_from_tl = tl_fetch_int();
      len -= sizeof(op_from_tl);
      assert(op_from_tl == op);
      assert(len % sizeof(int) == 0);

      if (op == TL_KPHP_START_LEASE) {
        static_assert(sizeof(process_id_t) == 12, "");

        if (len < sizeof(process_id_t) + sizeof(int)) {
          return 0;
        }

        union {
          std::array<char, sizeof(process_id_t)> buf;
          process_id_t xpid;
        };
        auto fetched_bytes = tl_fetch_data(buf.data(), buf.size());
        assert(fetched_bytes == buf.size());

        int timeout = tl_fetch_int();

        do_rpc_start_lease(xpid, precise_now + timeout);
        return 0;
      }

      auto req_id = tl_fetch_long();
      len -= sizeof(req_id);

      vkprintf(2, "got RPC_INVOKE_REQ [req_id = %016llx]\n", req_id);
      set_connection_timeout(c, script_timeout);

      char buf[len];
      auto fetched_bytes = tl_fetch_data(buf, len);
      assert(fetched_bytes == len);
      rpc_query_data *rpc_data = rpc_query_data_create(reinterpret_cast<int *>(buf), len / sizeof(int),
                                                       req_id, D->remote_pid.ip, D->remote_pid.port,
                                                       D->remote_pid.pid, D->remote_pid.utime);

      php_worker *worker = php_worker_create(run_once ? once_worker : rpc_worker, c, nullptr, rpc_data, script_timeout, req_id);
      D->extra = worker;

      c->status = conn_wait_net;
      rpcx_func_wakeup(c);
    }
  }

  return 0;
}


/***
  Common query function
 ***/

void pnet_query_answer(conn_query *q) {
  connection *req = q->requester;
  if (req != nullptr && req->generation == q->req_generation) {
    void *extra = nullptr;
    if (req->type == &ct_php_engine_rpc_server) {
      extra = TCP_RPC_DATA(req)->extra;
    } else if (req->type == &ct_php_rpc_client) {
      extra = TCP_RPC_DATA(req)->extra;
    } else if (req->type == &ct_php_engine_http_server) {
      extra = HTS_DATA (req)->extra;
    } else {
      assert ("unexpected type of connection\n" && 0);
    }
    php_worker_answer_query(reinterpret_cast<php_worker *>(extra), ((net_ansgen_t *)(q->extra))->ans);
  }
}

void pnet_query_delete(conn_query *q) {
  auto ansgen = (net_ansgen_t *)q->extra;

  ansgen->func->free(ansgen);
  q->extra = nullptr;

  delete_conn_query(q);
  zfree(q, sizeof(*q));
}

int pnet_query_timeout(conn_query *q) {
  auto net_ansgen = (net_ansgen_t *)q->extra;
  net_ansgen->func->timeout(net_ansgen);

  pnet_query_answer(q);

  delete_conn_query_from_requester(q);

  return 0;
}

int pnet_query_term(conn_query *q) {
  auto net_ansgen = (net_ansgen_t *)q->extra;
  net_ansgen->func->error(net_ansgen, "Connection closed by server");

  pnet_query_answer(q);
  pnet_query_delete(q);

  return 0;
}

int pnet_query_check(conn_query *q) {
  auto net_ansgen = (net_ansgen_t *)q->extra;

  ansgen_state_t state = net_ansgen->state;
  switch (state) {
    case st_ansgen_done:
    case st_ansgen_error:
      pnet_query_answer(q);
      pnet_query_delete(q);
      break;

    case st_ansgen_wait:
      break;
  }

  return state == st_ansgen_error;
}

int delayed_send_term(conn_query *q) {
  auto command = (command_t *)q->extra;

  if (command != nullptr) {
    command->run(command, nullptr);
    command->free(command);
    q->extra = nullptr;
  }

  delete_conn_query(q);
  zfree(q, sizeof(*q));
  return 0;
}

/***
  MEMCACHED CLIENT
 ***/

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
  auto ansgen = (mc_ansgen_t *)q->extra;
  ansgen->func->value(ansgen, reader);

  int err = pnet_query_check(q);
  return err;
}

int mc_query_other(conn_query *q, data_reader_t *reader) {
  auto ansgen = (mc_ansgen_t *)q->extra;
  ansgen->func->other(ansgen, reader);

  return pnet_query_check(q);
}

int mc_query_version(conn_query *q, data_reader_t *reader) {
  auto ansgen = (mc_ansgen_t *)q->extra;
  ansgen->func->version(ansgen, reader);

  return pnet_query_check(q);
}

int mc_query_end(conn_query *q) {
  auto ansgen = (mc_ansgen_t *)q->extra;
  ansgen->func->end(ansgen);

  return pnet_query_check(q);
}

int mc_query_xstored(conn_query *q, int is_stored) {
  auto ansgen = (mc_ansgen_t *)q->extra;
  ansgen->func->xstored(ansgen, is_stored);

  return pnet_query_check(q);
}

int mc_query_error(conn_query *q) {
  auto ansgen = (net_ansgen_t *)q->extra;
  ansgen->func->error(ansgen, "some protocol error");

  return pnet_query_check(q);
}

conn_query_functions pnet_cq_func = [] {
  auto res = conn_query_functions();
  res.magic = (int)CQUERY_FUNC_MAGIC;
  res.title = "pnet-cq-query";
  res.wakeup = pnet_query_timeout;
  res.close = pnet_query_term;
//  res.complete = mc_query_done //???
  return res;
}();

conn_query_functions pnet_delayed_cq_func = [] {
  auto res = conn_query_functions();
  res.magic = (int)CQUERY_FUNC_MAGIC;
  res.title = "pnet-cq-query";
  res.wakeup = pnet_query_term;
  res.close = pnet_query_term;
//  res.complete = mc_query_done //???

  return res;
}();

conn_query_functions delayed_send_cq_func = [] {
  auto res = conn_query_functions();
  res.magic = (int)CQUERY_FUNC_MAGIC;
  res.title = "delayed-send-cq-query";
  res.wakeup = delayed_send_term;
  res.close = delayed_send_term;
//  res.complete = mc_query_done //???

  return res;
}();


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

  vkprintf (1, "proxy_mc_client: op=%d, key_len=%d, arg#=%d, response_len=%d\n", op, D->key_len, D->arg_num, D->response_len);

  if (op == mcrt_empty) {
    return SKIP_ALL_BYTES;
  }

  conn_query *cur_query = nullptr;
  if (c->first_query == (conn_query *)c) {
    if (op != mcrt_VERSION) {
      vkprintf (-1, "response received for empty query list? op=%d\n", op);
      if (verbosity > -2) {
        dump_connection_buffers(c);
      }
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

    case mcrt_VALUE:
      if (D->key_len > 0 && D->key_len <= MAX_KEY_LEN && D->arg_num == 2 && (unsigned)D->args[1] <= MAX_VALUE_LEN) {
        int needed_bytes = (int)(D->args[1] + D->response_len + 2 - c->In.total_bytes);
        if (needed_bytes > 0) {
          return needed_bytes;
        }
        nbit_advance(&c->Q, (int)D->args[1]);
        len = nbit_ready_bytes(&c->Q);
        assert (len > 0);
        ptr = reinterpret_cast<char *>(nbit_get_ptr(&c->Q));
      } else {
        vkprintf (-1, "error at VALUE: op=%d, key_len=%d, arg_num=%d, value_len=%lld\n", op, D->key_len, D->arg_num, D->args[1]);

        D->response_flags |= 16;
        return SKIP_ALL_BYTES;
      }
      if (len == 1) {
        nbit_advance(&c->Q, 1);
      }
      if (ptr[0] != '\r' || (len > 1 ? ptr[1] : *((char *)nbit_get_ptr(&c->Q))) != '\n') {
        vkprintf (-1, "missing cr/lf at VALUE: op=%d, key_len=%d, arg_num=%d, value_len=%lld\n", op, D->key_len, D->arg_num, D->args[1]);

        assert (0);

        D->response_flags |= 16;
        return SKIP_ALL_BYTES;
      }
      len = 2;

      //vkprintf (1, "mcc_value: op=%d, key_len=%d, flags=%lld, time=%lld, value_len=%lld\n", op, D->key_len, D->args[0], D->args[1], D->args[2]);

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
      assert (advance_skip_read_ptr(&c->In, query_len) == query_len);
      return 0;

    case mcrt_VERSION:
      c->unreliability >>= 1;
      vkprintf (3, "mcc_got_version: op=%d, key_len=%d, unreliability=%d\n", op, D->key_len, c->unreliability);

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
      vkprintf (-1, "CLIENT_ERROR received from connection %d (%s)\n", c->fd, sockaddr_storage_to_string(&c->remote_endpoint));
      //client_errors_received++;
      /* fallthrough */
    case mcrt_ERROR:
      //errors_received++;
      /*if (verbosity > -2 && errors_received < 32) {
        dump_connection_buffers (c);
        if (c->first_query != (conn_query *) c && c->first_query->req_generation == c->first_query->requester->generation) {
          dump_connection_buffers (c->first_query->requester);
        }
      }*/

      fail_connection(c, -5);
      c->ready = cr_failed;
      //conn_query will be closed during connection closing
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
      assert (advance_skip_read_ptr(&c->In, query_len) == query_len);
      return 0;
      break;

    case mcrt_END:
      mc_query_end(c->first_query);
      return SKIP_ALL_BYTES;

    default:
      assert ("unknown state" && 0);
  }

  assert ("unreachable position" && 0);
  return 0;
}


/***
  DB CLIENT
 ***/
int sql_query_packet(conn_query *q, data_reader_t *reader) {
  auto ansgen = (sql_ansgen_t *)q->extra;
  ansgen->func->packet(ansgen, reader);

  return pnet_query_check(q);
}

int sql_query_done(conn_query *q) {
  auto ansgen = (sql_ansgen_t *)q->extra;
  ansgen->func->done(ansgen);

  return pnet_query_check(q);
}

void create_pnet_delayed_query(connection *http_conn, conn_target_t *t, net_ansgen_t *gen, double finish_time) {
  auto q = reinterpret_cast<conn_query *>(zmalloc(sizeof(conn_query)));

  q->custom_type = 0;
  q->outbound = nullptr;
  q->requester = http_conn;
  q->start_time = precise_now;

  q->extra = gen;

  q->cq_type = &pnet_delayed_cq_func;
  q->timer.wakeup_time = finish_time;

  insert_conn_query_into_list(q, (conn_query *)t);
}

void create_delayed_send_query(conn_target_t *t, command_t *command,
                               double finish_time) {
  auto q = reinterpret_cast<conn_query *>(zmalloc(sizeof(conn_query)));

  q->custom_type = 0;
  q->start_time = precise_now;

  q->outbound = nullptr;
  q->requester = nullptr;
  q->extra = command;

  q->cq_type = &delayed_send_cq_func;
  q->timer.wakeup_time = finish_time;

  insert_conn_query_into_list(q, (conn_query *)t);
}

conn_query *create_pnet_query(connection *http_conn, connection *conn, net_ansgen_t *gen, double finish_time) {
  auto q = reinterpret_cast<conn_query *>(zmalloc(sizeof(conn_query)));

  q->custom_type = 0;
  q->outbound = conn;
  q->requester = http_conn;
  q->start_time = precise_now;

  q->extra = gen;

  q->cq_type = &pnet_cq_func;
  q->timer.wakeup_time = finish_time;

  insert_conn_query(q);

  return q;
}

int sqlp_becomes_ready(connection *c) {
  conn_query *q;


  while (c->target->first_query != (conn_query *)(c->target)) {
    q = c->target->first_query;
    //    fprintf (stderr, "processing delayed query %p for target %p initiated by %p (%d:%d<=%d)\n", q, c->target, q->requester, q->requester->fd, q->req_generation, q->requester->generation);
    if (q->requester != nullptr && q->requester->generation == q->req_generation) {
      q->requester->queries_ok++;
      //waiting_queries--;

      auto net_ansgen = (net_ansgen_t *)q->extra;
      create_pnet_query(q->requester, c, net_ansgen, q->timer.wakeup_time);

      delete_conn_query(q);
      zfree(q, sizeof(*q));

      auto ansgen = (sql_ansgen_t *)net_ansgen;
      ansgen->func->ready(ansgen, c);
      break;
    } else {
      //waiting_queries--;
      pnet_query_delete(q);
    }
  }
  return 0;
}

int sqlp_check_ready(connection *c) {
  if (c->status == conn_ready && c->In.total_bytes > 0) {
    vkprintf (-1, "have %d bytes in outbound sql connection %d in state ready, closing connection\n", c->In.total_bytes, c->fd);
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
    if (!(c->flags & C_FAILED) && c->last_query_sent_time < precise_now - RESPONSE_FAIL_TIMEOUT - c->last_query_time && c->last_response_time < precise_now - RESPONSE_FAIL_TIMEOUT - c
      ->last_query_time && !(SQLC_DATA(c)->extra_flags & 1)) {
      vkprintf (1, "failing outbound connection %d, status=%d, response_status=%d, last_response=%.6f, last_query=%.6f, now=%.6f\n", c->fd, c->status, SQLC_DATA(c)
        ->response_state, c->last_response_time, c->last_query_sent_time, precise_now);
      c->error = -5;
      fail_connection(c, -5);
      return c->ready = cr_failed;
    }
  }
  return c->ready = (c->status == conn_ready ? cr_ok : (SQLC_DATA(c)->auth_state == sql_auth_ok ? cr_stopped : cr_busy));  /* was cr_busy instead of cr_stopped */
}


int sqlp_authorized(connection *c) {
  nbw_iterator_t it;
  unsigned temp = 0x00000000;
  int len = 0;
  char ptype = 2;

  if (!sql_database || !*sql_database) {
    SQLC_DATA(c)->auth_state = sql_auth_ok;
    c->status = conn_ready;
    SQLC_DATA(c)->packet_state = 0;
    vkprintf (1, "outcoming initdb successful\n");
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

  vkprintf (1, "proxy_db_client: op=%d, packet_len=%d, response_state=%d, field_num=%d\n", op, D->packet_len, D->response_state, field_cnt);

  if (c->first_query == (conn_query *)c) {
    vkprintf (-1, "response received for empty query list? op=%d\n", op);
    return SKIP_ALL_BYTES;
  }

  c->last_response_time = precise_now;

  int query_len = D->packet_len + 4;
  data_reader_t *reader = create_data_reader(c, query_len);

  int x;
  switch (D->response_state) {
    case resp_first:
      //forward_response (c, D->packet_len, SQLC_DATA(c)->extra_flags & 1);
      x = sql_query_packet(c->first_query, reader);

      if (!reader->readed) {
        assert (advance_skip_read_ptr(&c->In, query_len) == query_len);
      }

      if (x) {
        fail_connection(c, -6);
        c->ready = cr_failed;
        return 0;
      }

      if (field_cnt == 0 && (SQLC_DATA(c)->extra_flags & 1)) {
        dl_unreachable ("looks like unused code");
        SQLC_DATA(c)->extra_flags |= 2;
        if (c->first_query->requester->generation != c->first_query->req_generation) {
          vkprintf (1, "outbound connection %d: nowhere to forward replication stream, closing\n", c->fd);
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
      //forward_response (c, D->packet_len, 0);
      x = sql_query_packet(c->first_query, reader);
      if (!reader->readed) {
        assert (advance_skip_read_ptr(&c->In, query_len) == query_len);
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
      //forward_response (c, D->packet_len, 0);
      x = sql_query_packet(c->first_query, reader);
      if (!reader->readed) {
        assert (advance_skip_read_ptr(&c->In, query_len) == query_len);
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
      vkprintf (-1, "unexpected packet from server!\n");
      assert (0);
  }

  if (D->response_state == resp_done) {
//    query_complete (c, 1);
    x = sql_query_done(c->first_query);
    if (x) {
      fail_connection(c, -9);
      c->ready = cr_failed;
      return 0;
    }

    //active_queries--;
    c->unreliability >>= 1;
    c->status = conn_ready;
    c->last_query_time = precise_now - c->last_query_sent_time;
    //SQLC_DATA(c)->extra_flags &= -2;
    sqlp_becomes_ready(c);
  }

  return 0;
}

/***
  RCP CLIENT
 ***/
int rpcc_check_ready(connection *c) {
  /*assert (c->status != conn_none);*/
  /*if (c->status == conn_none || c->status == conn_connecting || TCP_RPC_DATA(c)->in_packet_num < 0) {*/
  /*return c->ready = cr_notyet;*/
  /*}*/
  /*if (c->status == conn_error || c->ready == cr_failed) {*/
  /*return c->ready = cr_failed;*/
  /*}*/
  /*return c->ready = cr_ok;*/

  if ((c->flags & C_FAILED) || c->status == conn_error) {
    if (verbosity > 1 && c->ready != cr_failed) {
      fprintf(stderr, "changing connection %d readiness from %d to %d [FAILED] lq=%.03f lr=%.03f now=%.03f\n", c->fd, c->ready, cr_failed, c->last_query_sent_time, c
        ->last_response_time, precise_now);
    }
    return c->ready = cr_failed;
  }

  if (TCP_RPC_DATA(c)->in_packet_num < 0) {
    if (TCP_RPC_DATA(c)->in_packet_num == -1 && c->status == conn_running) {
      if (verbosity > 1 && c->ready != cr_ok) {
        fprintf(stderr, "changing connection %d readiness from %d to %d [OK] lq=%.03f lr=%.03f now=%.03f\n", c->fd, c->ready, cr_ok, c->last_query_sent_time, c
          ->last_response_time, precise_now);
      }
      return c->ready = cr_ok;
    }

    assert (c->last_query_sent_time != 0);
    if (c->last_query_sent_time < precise_now - RPC_CONNECT_TIMEOUT) {
      fail_connection(c, -6);
      return c->ready = cr_failed;
    }
    return c->ready = cr_notyet;
  }
  assert (c->status != conn_connecting);

  if (c->last_query_sent_time < precise_now - rpc_ping_interval) {
    net_rpc_send_ping(c, lrand48());
    c->last_query_sent_time = precise_now;
  }

  assert (c->last_response_time != 0);
  if (c->last_response_time < precise_now - RPC_FAIL_INTERVAL) {
    if (verbosity > 1 && c->ready != cr_failed) {
      fprintf(stderr, "changing connection %d readiness from %d to %d [FAILED] lq=%.03f lr=%.03f now=%.03f\n", c->fd, c->ready, cr_failed, c->last_query_sent_time, c
        ->last_response_time, precise_now);
    }
    fail_connection(c, -5);
    return c->ready = cr_failed;
  }

  if (c->last_response_time < precise_now - RPC_STOP_INTERVAL) {
    if (verbosity > 1 && c->ready != cr_stopped) {
      fprintf(stderr, "changing connection %d readiness from %d to %d [STOPPED] lq=%.03f lr=%.03f now=%.03f\n", c->fd, c->ready, cr_stopped, c->last_query_sent_time, c
        ->last_response_time, precise_now);
    }
    return c->ready = cr_stopped;
  }

  if (c->status == conn_wait_answer || c->status == conn_expect_query || c->status == conn_reading_answer) {
    if (verbosity > 1 && c->ready != cr_ok) {
      fprintf(stderr, "changing connection %d readiness from %d to %d [OK] lq=%.03f lr=%.03f now=%.03f\n", c->fd, c->ready, cr_ok, c->last_query_sent_time, c->last_response_time, precise_now);
    }
    return c->ready = cr_ok;
  }

  if (c->status == conn_running) {
    if (verbosity > 1 && c->ready != cr_busy) {
      fprintf(stderr, "changing connection %d readiness from %d to %d [BUSY] lq=%.03f lr=%.03f now=%.03f\n", c->fd, c->ready, cr_busy, c->last_query_sent_time, c
        ->last_response_time, precise_now);
    }
    return c->ready = cr_busy;
  }

  assert (0);
  return c->ready = cr_failed;
}

int rpcc_send_query(connection *c) {
  //rpcc_ready
  c->last_query_sent_time = precise_now;
  c->last_response_time = precise_now;

  conn_query *q;
  dl_assert (c != nullptr, "...");
  dl_assert (c->target != nullptr, "...");

  while (c->target->first_query != (conn_query *)(c->target)) {
    q = c->target->first_query;
    dl_assert (q != nullptr, "...");
//    dl_assert (q->requester != nullptr, "...");
//    fprintf (stderr, "processing delayed query %p for target %p initiated by %p (%d:%d<=%d)\n", q, c->target, q->requester, q->requester->fd, q->req_generation, q->requester->generation);

    auto command = (command_t *)q->extra;
    command->run(command, c);
    command->free(command);
    q->extra = nullptr;
    delete_conn_query(q);
    zfree(q, sizeof(*q));
  }
  return 0;
}

int php_rpcc_execute(connection *c, int op, raw_message *raw) {
  vkprintf (1, "php_rpcc_execute: fd=%d, op=%d, len=%d\n", c->fd, op, raw->total_bytes);

  int event_status = 0;
  c->last_response_time = precise_now;

  switch (static_cast<unsigned int>(op)) {
    case TL_RPC_REQ_ERROR:
    case TL_RPC_REQ_RESULT: {
      int result_len = raw->total_bytes - sizeof(int) - sizeof(long long);
      assert(result_len >= 0);

      tl_fetch_init_tcp_raw_message(raw, raw->total_bytes);

      auto op_from_tl = tl_fetch_int();
      assert(op_from_tl == op);

      auto id = tl_fetch_long();
      if (op == TL_RPC_REQ_ERROR) {
        //FIXME: error code, error string
        //almost never happens
        event_status = create_rpc_error_event(id, -1, "unknown error", nullptr);
        break;
      }

      net_event_t *event = nullptr;
      event_status = create_rpc_answer_event(id, result_len, &event);
      if (event_status > 0) {
        auto fetched_bytes = tl_fetch_data(event->result, result_len);
        assert (fetched_bytes == result_len);
      }

      break;
    }
    case TL_RPC_PONG:
      break;
  }

  if (event_status) {
    on_net_event(event_status);
  }

  return 0;
}

static char stats[65536];
static int stats_len;

void prepare_full_stats() {
  char *s = stats;
  int s_left = 65530;

  int uptime = get_uptime();
  worker_acc_stats.tot_idle_time = epoll_total_idle_time();
  worker_acc_stats.tot_idle_percent =
    uptime > 0 ? epoll_total_idle_time() / uptime * 100 : 0;
  worker_acc_stats.a_idle_percent = epoll_average_idle_quotient() > 0 ?
                                    epoll_average_idle_time() / epoll_average_idle_quotient() * 100 : 0;
  size_t acc_stats_size = sizeof(worker_acc_stats);
  assert (s_left > acc_stats_size);
  memcpy(s, &worker_acc_stats, acc_stats_size);
  s += acc_stats_size;
  s_left -= (int)acc_stats_size;

#define W(args...)  ({\
  int written_tmp___ = snprintf (s, (size_t)s_left, ##args);\
  if (written_tmp___ > s_left - 1) {\
    written_tmp___ = s_left - 1;\
  }\
  s += written_tmp___;\
  s_left -= written_tmp___;\
})

  int pid = (int)getpid();
  W ("pid %d\t%d\n", pid, pid);
  W ("active_special_connections %d\t%d\n", pid, active_special_connections);
  W ("max_special_connections %d\t%d\n", pid, max_special_connections);
//  W ("TODO: more stats\n");
#undef W
  stats_len = (int)(s - stats);
}

/***
  SERVER
 ***/

static void sigint_immediate_handler(const int sig __attribute__((unused))) {
  const char message[] = "SIGINT handled immediately.\n";
  kwrite(2, message, sizeof(message) - (size_t)1);

  _exit(1);
}

static void sigterm_immediate_handler(const int sig __attribute__((unused))) {
  const char message[] = "SIGTERM handled immediately.\n";
  kwrite(2, message, sizeof(message) - (size_t)1);

  _exit(1);
}

static void sigint_handler(const int sig) {
  const char message[] = "SIGINT handled.\n";
  kwrite(2, message, sizeof(message) - (size_t)1);

  pending_signals |= (1 << sig);
  ksignal(sig, sigint_immediate_handler);
}

static void turn_sigterm_on() {
  if (sigterm_on != 1) {
    sigterm_time = precise_now + SIGTERM_MAX_TIMEOUT;
    sigterm_on = 1;
  }
}

static void sigterm_handler(const int sig) {
  const char message[] = "SIGTERM handled.\n";
  kwrite(2, message, sizeof(message) - (size_t)1);

  //pending_signals |= (1 << sig);
  turn_sigterm_on();
  ksignal(sig, sigterm_immediate_handler);
}

static void sighup_handler(const int sig) {
  const char message[] = "got SIGHUP.\n";
  kwrite(2, message, sizeof(message) - (size_t)1);

  pending_signals |= (1ll << sig);
}

static void sigusr1_handler(const int sig) {
  const char message[] = "got SIGUSR1, rotate logs.\n";
  kwrite(2, message, sizeof(message) - (size_t)1);

  pending_signals |= (1ll << sig);
}

int pipe_packet_id = 0;

void write_full_stats_to_pipe() {
  if (master_pipe_write != -1) {
    prepare_full_stats();

    int qsize = stats_len + 1 + (int)sizeof(int) * 5;
    qsize = (qsize + 3) & -4;
    auto q = reinterpret_cast<int *>(malloc((size_t)qsize));
    memset(q, 0, (size_t)qsize);

    q[2] = RPC_PHP_FULL_STATS;
    memcpy(q + 3, stats, (size_t)stats_len);

    prepare_rpc_query_raw(pipe_packet_id++, q, qsize, crc32c_partial);
    int err = (int)write(master_pipe_write, q, (size_t)qsize);
    dl_passert (err == qsize, dl_pstr("error during write to pipe [%d]\n", master_pipe_write));
    if (err != qsize) {
      kill(getppid(), 9);
    }
    free(q);
  }
}

sig_atomic_t pipe_fast_packet_id = 0;

void write_immediate_stats_to_pipe() {
  if (master_pipe_fast_write != -1) {
#define QSIZE (sizeof (php_immediate_stats_t) + 1 + sizeof (int) * 5 + 3) & -4
    int q[QSIZE] = {0};
    int qsize = QSIZE;
#undef QSIZE

    q[2] = RPC_PHP_IMMEDIATE_STATS;
    memcpy(q + 3, get_imm_stats(), sizeof(php_immediate_stats_t));

    prepare_rpc_query_raw(pipe_fast_packet_id++, q, qsize, crc32c_partial);
    int err = (int)write(master_pipe_fast_write, q, (size_t)qsize);
    dl_passert (err == qsize, dl_pstr("error [%d] during write to pipe", errno));
  }
}

//Used for interaction with master.
int spoll_send_stats;

static void sigstats_handler(int signum __attribute__((unused)), siginfo_t *info, void *data __attribute__((unused))) {
  dl_assert (info != nullptr, "SIGPOLL with no info");
  if (info->si_code == SI_QUEUE) {
    int code = info->si_value.sival_int;
    if ((code & 0xFFFF0000) == SPOLL_SEND_STATS) {
      if (code & SPOLL_SEND_FULL_STATS) {
        spoll_send_stats++;
      }
      if (code & SPOLL_SEND_IMMEDIATE_STATS) {
        write_immediate_stats_to_pipe();
      }
    }
  }
}

void cron() {
  if (master_flag == -1 && getppid() == 1) {
    turn_sigterm_on();
  }
}

int try_get_http_fd() {
  return server_socket(http_port, settings_addr, backlog, 0);
}

void start_server() {
  int prev_time;
  double next_create_outbound = 0;

  if (run_once) {
    master_flag = 0;
    rpc_port = -1;
    http_port = -1;
    rpc_client_port = -1;
    setvbuf(stdout, nullptr, _IONBF, 0);
  }

  pending_signals = 0;
  if (daemonize) {
    setsid();

    ksignal(SIGHUP, sighup_handler);
    reopen_logs();
  }
  if (master_flag) {
    vkprintf (-1, "master\n");
    if (rpc_port != -1) {
      vkprintf (-1, "rpc_port is ignored in master mode\n");
      rpc_port = -1;
    }
  }

  init_netbuffers();

  init_epoll();
  if (master_flag) {
    start_master(http_port > 0 ? &http_sfd : nullptr, &try_get_http_fd, http_port);

    if (logname_pattern != nullptr) {
      reopen_logs();
    }
  } else {
    if (logname != nullptr) {
      kstdout = dup(1);
      if (kstdout <= 2) {
        kprintf("Can't save stdout before opening logs\n");
        exit(1);
      }
      kstderr = dup(2);
      if (kstderr <= 2) {
        kprintf("Can't save stderr before opening logs\n");
        exit(1);
      }
      reopen_logs();
    }
  }

  prev_time = 0;

  if (http_port > 0 && http_sfd < 0) {
    dl_assert (!master_flag, "failed to get http_fd\n");
    if (master_flag) {
      vkprintf (-1, "try_get_http_fd after start_master\n");
      exit(1);
    }
    http_sfd = try_get_http_fd();
    if (http_sfd < 0) {
      vkprintf (-1, "cannot open http server socket at port %d: %m\n", http_port);
      exit(1);
    }
  }

  if (rpc_port > 0 && rpc_sfd < 0) {
    rpc_sfd = server_socket(rpc_port, settings_addr, backlog, 0);
    if (rpc_sfd < 0) {
      vkprintf (-1, "cannot open rpc server socket at port %d: %m\n", rpc_port);
      exit(1);
    }
  }

  if (verbosity > 0 && http_sfd >= 0) {
    vkprintf (-1, "created listening socket at %s:%d, fd=%d\n", ip_to_print(settings_addr.s_addr), port, http_sfd);
  }

  if (http_sfd >= 0) {
    init_listening_tcpv6_connection(http_sfd, &ct_php_engine_http_server, &http_methods, SM_SPECIAL);
  }

  if (rpc_sfd >= 0) {
    init_listening_connection(rpc_sfd, &ct_php_engine_rpc_server, &rpc_methods);
  }

  if (no_sql) {
    sql_target_id = -1;
  } else {
    sql_target_id = get_target("localhost", db_port, &db_ct);
    assert (sql_target_id != -1);
  }

  if ((rpc_client_host != nullptr) ^ (rpc_client_port != -1)) {
    vkprintf (-1, "warning: only rpc_client_host or rpc_client_port is defined\n");
  }
  if (rpc_client_host != nullptr && rpc_client_port != -1) {
    vkprintf (-1, "create rpc client target: %s:%d\n", rpc_client_host, rpc_client_port);
    set_main_target(get_target(rpc_client_host, rpc_client_port, &rpc_client_ct));
  }

  if (run_once) {
    int pipe_fd[2];
    pipe(pipe_fd);

    int read_fd = pipe_fd[0];
    int write_fd = pipe_fd[1];

    rpc_client_methods.rpc_ready = nullptr;
    create_pipe_reader(read_fd, &ct_php_rpc_client, &rpc_client_methods);

    int q[6];
    int qsize = 6 * sizeof(int);
    q[2] = TL_RPC_INVOKE_REQ;
    for (int i = 0; i < run_once_count; i++) {
      prepare_rpc_query_raw(i, q, qsize, crc32c_partial);
      assert (write(write_fd, q, (size_t)qsize) == qsize);
    }
  }

  get_utime_monotonic();
  //create_all_outbound_connections();

  ksignal(SIGTERM, sigterm_handler);
  ksignal(SIGPIPE, SIG_IGN);
  ksignal(SIGINT, run_once ? sigint_immediate_handler : sigint_handler);
  ksignal(SIGUSR1, sigusr1_handler);
  ksignal(SIGPOLL, SIG_IGN);

  //using sigaction for SIGSTAT
  assert (SIGRTMIN <= SIGSTAT && SIGSTAT <= SIGRTMAX);
  dl_sigaction(SIGSTAT, nullptr, dl_get_empty_sigset(), SA_SIGINFO | SA_ONSTACK | SA_RESTART, sigstats_handler);

  dl_allow_all_signals();

  vkprintf (1, "Server started\n");
  for (int i = 0; !(pending_signals & ~((1ll << SIGUSR1) | (1ll << SIGHUP))); i++) {
    if (verbosity > 0 && !(i & 255)) {
      vkprintf (1, "epoll_work(): %d out of %d connections, network buffers: %d used, %d out of %d allocated\n",
                active_connections, maxconn, NB_used, NB_alloc, NB_max);
    }
    epoll_work(57);

    if (precise_now > next_create_outbound) {
      create_all_outbound_connections();
      next_create_outbound = precise_now + 0.03 + 0.02 * drand48();
    }

    while (spoll_send_stats > 0) {
      write_full_stats_to_pipe();
      spoll_send_stats--;
    }

    if (pending_signals & (1ll << SIGHUP)) {
      pending_signals &= ~(1ll << SIGHUP);
    }

    if (pending_signals & (1ll << SIGUSR1)) {
      pending_signals &= ~(1ll << SIGUSR1);

      reopen_logs();
    }

    if (now != prev_time) {
      prev_time = now;
      cron();
    }

    lease_cron();
    if (sigterm_on && !rpc_stopped) {
      rpcc_stop();
    }
    if (sigterm_on && !hts_stopped) {
      hts_stop();
    }

    if (main_thread_reactor.pre_event) {
      main_thread_reactor.pre_event();
    }

    if (sigterm_on && precise_now > sigterm_time && !php_worker_run_flag &&
        pending_http_queue.first_query == (conn_query *)&pending_http_queue) {
      vkprintf (1, "Quitting because of sigterm\n");
      break;
    }
  }

  if (verbosity > 0 && pending_signals) {
    vkprintf (1, "Quitting because of pending signals = %llx\n", pending_signals);
  }

  if (http_sfd >= 0) {
    epoll_close(http_sfd);
    assert (close(http_sfd) >= 0);
  }
}

void set_instance_cache_memory_limit(dl::size_type limit);
void init_php_scripts();
void global_init_php_scripts();

void init_all() {
  srand48((long)cycleclock_now());

  //init pending_http_queue
  pending_http_queue.first_query = pending_http_queue.last_query = (conn_query *)&pending_http_queue;
  php_worker_run_flag = 0;

  dummy_request_queue.first_query = dummy_request_queue.last_query = (conn_query *)&dummy_request_queue;

  if (script_timeout == 0) {
    script_timeout = run_once ? 1e6 : DEFAULT_SCRIPT_TIMEOUT;
  }

  global_init_runtime_libs();
  global_init_php_scripts();
  global_init_script_allocator();

  init_handlers();

  init_drivers();

  init_php_scripts();
  idle_server_status();
  custom_server_status("<none>", 6);
  server_status_rpc(0, 0, dl_time());

  worker_id = (int)lrand48();
}

/*
 *
 *    MAIN
 *
 */

void init_logname(const char *src) {
  const char *t = src;
  while (*t && *t != '%') {
    t++;
  }
  int has_percent = (*t == '%');
  if (!has_percent) {
    logname = src;
    kprintf_multiprocessing_mode_enable();
    return;
  }

  char buf1[100];
  char buf2[100];
  int buf_len = 100;

  char *patt = buf1;
  char *plane = buf2;

  int was_percent = 0;
  while (*src) {
    assert (patt < buf1 + buf_len - 3);
    if (*src == '%') {
      if (!was_percent) {
        *patt++ = '%';
        *patt++ = 'd';
        was_percent = 1;
      }
    } else {
      *patt++ = *src;
      *plane++ = *src;
    }
    src++;
  }
  *patt = 0;
  patt = buf1;
  *plane = 0;
  plane = buf2;

  logname = strdup(plane);
  logname_pattern = strdup(patt);
}

/** main arguments parsing **/
int main_args_handler(int i) {
  switch (i) {
    case 'D': {
      char *key = optarg, *value;
      char *eq = strchr(key, '=');
      if (eq == nullptr) {
        kprintf ("-D option, can't find '='\n");
        return -1;
      }
      value = eq + 1;
      *eq = 0;
      ini_set(key, value);
      *eq = '=';
      return 0;
    }
    case 'l': {
      init_logname(optarg);
      return 0;
    }
    case 'H': {
      http_port = atoi(optarg);
      return 0;
    }
    case 'r': {
      rpc_port = atoi(optarg);
      return 0;
    }
    case 'R': {
      force_clear_sql_connection = 1;
      return 0;
    }
    case 'L': {
      static_buffer_length_limit = parse_memory_limit_default(optarg, 'm');
      return 0;
    }
    case 'w': {
      char *colon = strrchr(optarg, ':');
      if (colon == nullptr) {
        kprintf ("-w option, can't find ':'\n");
        return -1;
      }
      rpc_client_host = strndup(optarg, colon - optarg);
      char *at = strchr(colon, '@');
      if (at) {
        *at = 0;
      }
      rpc_client_port = atoi(colon + 1);
      if (at) {
        *at = '@';
        rpc_client_actor = atoi(at + 1);
      }
      return 0;
    }
    case 'E': {
      read_engine_tag(optarg);
      return 0;
    }
    case 'm': {
      max_memory = parse_memory_limit_default(optarg, 'm');
      assert((1 << 20) <= max_memory && max_memory <= (2047LL << 20));
      return 0;
    }
    case 'f': {
      workers_n = atoi(optarg);
      if (workers_n >= 0) {
        if (workers_n > MAX_WORKERS) {
          workers_n = MAX_WORKERS;
        }
        master_flag = 1;
      }
      return 0;
    }
    case 'p': {
      master_port = atoi(optarg);
      return 0;
    }
    case 's': {
      cluster_name = optarg;
      return 0;
    }
    case 'T': {
      // do nothing, tl schema is compiled, not provided in runtime
      kprintf("Ignoring tl schema: kphp codegenerates and compiles it\n");
      return 0;
    }
    case 't': {
      script_timeout = atoi(optarg);
      if (script_timeout < 1 || script_timeout > MAX_SCRIPT_TIMEOUT) {
        kprintf("Bad script timeout, schould be [%d..%d]", 1, MAX_SCRIPT_TIMEOUT);
      }
      if (script_timeout < 1) {
        script_timeout = 1;
      }
      if (script_timeout > MAX_SCRIPT_TIMEOUT) {
        script_timeout = MAX_SCRIPT_TIMEOUT;
      }
      return 0;
    }
    case 'o': {
      run_once = 1;
      return 0;
    }
    case 'q': {
      no_sql = 1;
      return 0;
    }
    case 'Q': {
      db_port = atoi(optarg);
      if (!(1000 <= db_port && db_port <= 0xffff)) {
        kprintf("-Q option: %d is strange port\n", db_port);
        return -1;
      }
      return 0;
    }
    case 'U': {
      if (optarg) {
        disable_access_log += atoi(optarg);
      } else {
        disable_access_log++;
      }
      return 0;
    }
    case 'K': {
      die_on_fail = 1;
      return 0;
    }
    case 'k': {
      if (mlockall(MCL_CURRENT | MCL_FUTURE) != 0) {
        kprintf ("error: fail to lock paged memory\n");
      }
      return 0;
    }
    case 2000: {
      queries_to_recreate_script = atoi(optarg);
      if (queries_to_recreate_script <= 0) {
        kprintf("worker-queries-to-reload has to be positive\n");
        return -1;
      }
      return 0;
    }
    case 2001: {
      memory_used_to_recreate_script = parse_memory_limit(optarg);
      // Align to page boundary
      const size_t page_size = 4096;
      memory_used_to_recreate_script = ((memory_used_to_recreate_script + (page_size - 1)) / page_size) * page_size;
      if (memory_used_to_recreate_script <= 0) {
        kprintf("couldn't parse worker-memory-to-reload argument\n");
        return -1;
      }
      return 0;
    }
    case 2002: {
      use_madvise_dontneed = 1;
      return 0;
    }
    case 2003: {
      int64_t instance_cache_memory_limit = parse_memory_limit(optarg);
      if (instance_cache_memory_limit <= 0 || instance_cache_memory_limit > std::numeric_limits<dl::size_type>::max()) {
        kprintf("couldn't parse instance-cache-memory-limit argument\n");
        return -1;
      }
      set_instance_cache_memory_limit(static_cast<dl::size_type>(instance_cache_memory_limit));
      return 0;
    }
    default:
      return -1;
  }
}

void parse_main_args_till_option(int argc, char *argv[], const char *till_option = nullptr) {
  if (run_once) {
    arg_add(argv[0]);
    while (optind != argc) {
      const char *opt = argv[optind++];
      if (till_option && !strcmp(till_option, opt)) {
        return;
      }
      arg_add(opt);
    }
  } else if (argc != optind) {
    usage_and_exit();
  }
}

void parse_main_args(int argc, char *argv[]) {
  usage_set_other_args_desc("");
  option_section_t sections[] = {OPT_GENERIC, OPT_NETWORK, OPT_RPC, OPT_VERBOSITY, OPT_ENGINE_CUSTOM, OPT_ARRAY_END};
  init_parse_options(sections);

  remove_parse_option("log");
  remove_parse_option("port");
  remove_parse_option("clusters-config");
  always_enable_option("maximize-tcp-buffers", NULL);
  parse_option("log", required_argument, 'l', "set log name. %% can be used for log-file per worker");
  parse_option("lock-memory", no_argument, 'k', "lock paged memory");
  parse_option("define", required_argument, 'D', "set data for ini_get (in form key=value)");
  parse_option("http-port", required_argument, 'H', "http port");
  parse_option("rpc-port", required_argument, 'r', "rpc port");
  parse_option("rpc-client", required_argument, 'w', "host and port for client mode (host:port)");
  parse_option("hard-memory-limit", required_argument, 'm', "maximal size of memory used by script");
  parse_option("force-clear-sql", no_argument, 'R', "force clear sql connection every script run");
  parse_option("disable-sql", no_argument, 'q', "disable using sql");
  parse_option("sql-port", required_argument, 'Q', "sql port");
  parse_option("static-buffers-size", required_argument, 'L', "limit for static buffers length (e.g. limits script output size)");
  parse_option("error-tag", required_argument, 'E', "name of file with engine tag showed on every warning");
  parse_option("workers-num", required_argument, 'f', "run workers_n workers");
  parse_option("once", no_argument, 'o', "run script once");
  parse_option("master-port", required_argument, 'p', "port for memcached interface to master");
  parse_option("cluster-name", required_argument, 's', "only one kphp with same cluster name will be run on one machine");
  parse_option("tl-schema", required_argument, 'T', "(deprecated) name of file with TL config (will be ignored)");
  parse_option("time-limit", required_argument, 't', "time limit for script in seconds");
  parse_option_alias("crc32c", 'C');
  parse_option("small-acsess-log", optional_argument, 'U', "don't write get data in log. If used twice (or with value 2), disables access log.");
  parse_option("fatal-warnings", no_argument, 'K', "script is killed, when warning happened");
  parse_option("worker-queries-to-reload", required_argument, 2000, "worker script is reloaded, when <queries> queries processed (default: 100)");
  parse_option("worker-memory-to-reload", required_argument, 2001, "worker script is reloaded, when <memory> queries processed");
  parse_option("use-madvise-dontneed", no_argument, 2002, "Use madvise MADV_DONTNEED for script memory above limit");
  parse_option("instance-cache-memory-limit", required_argument, 2003, "memory limit for instance_cache");
  parse_engine_options_long(argc, argv, main_args_handler);
  parse_main_args_till_option(argc, argv);
}


void init_default() {
  dl_set_default_handlers();
  now = (int)time(nullptr);

  pid = getpid();
  // RPC part
  PID.port = (short)rpc_port;

  dynamic_data_buffer_size = (1 << 26);//26 for struct conn_query
  init_dyn_data();

  if (!username && maxconn == MAX_CONNECTIONS && geteuid()) {
    maxconn = 1000; //not for root
  }

  if (raise_file_rlimit(maxconn + 16) < 0) {
    vkprintf (-1, "fatal: cannot raise open file limit to %d\n", maxconn + 16);
    exit(1);
  }

  aes_load_keys();

  do_relogin();
  prctl(PR_SET_DUMPABLE, 1);
}

int run_main(int argc, char **argv, php_mode mode) {
  init_version_string(NAME_VERSION);
  dl_block_all_signals();
#ifndef __SANITIZE_ADDRESS__
  set_core_dump_rlimit(1LL << 40);
#endif
  max_special_connections = 1;
  static_assert(offsetof(tcp_rpc_client_functions, rpc_ready) == offsetof(tcp_rpc_server_functions, rpc_ready), "");

  if (mode == php_mode::cli) {
    run_once = 1;
    disable_access_log = 2;
    parse_main_args_till_option(argc, argv, "--Xkphp-options");
  }

  parse_main_args(argc, argv);

  load_time = -dl_time();

  init_default();

  init_all();

  load_time += dl_time();

  init_uptime();
  preallocate_msg_buffers();

  start_server();

  vkprintf (1, "return 0;\n");
  if (run_once) {
    return run_once_return_code;
  }
  return 0;
}
