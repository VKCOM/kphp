// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "server/php-engine.h"

#include <cassert>
#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <limits>
#include <netdb.h>
#include <netinet/in.h>
#include <poll.h>
#include <re2/re2.h>
#include <sys/mman.h>
#include <sys/socket.h>
#include <unistd.h>

#include "common/algorithms/clamp.h"
#include "common/algorithms/find.h"
#include "common/crc32c.h"
#include "common/cycleclock.h"
#include "common/dl-utils-lite.h"
#include "common/kprintf.h"
#include "common/macos-ports.h"
#include "common/options.h"
#include "common/pipe-utils.h"
#include "common/precise-time.h"
#include "common/resolver.h"
#include "common/rpc-error-codes.h"
#include "common/sanitizer.h"
#include "common/server/limits.h"
#include "common/server/relogin.h"
#include "common/server/signals.h"
#include "common/tl/constants/common.h"
#include "common/tl/constants/kphp.h"
#include "common/tl/methods/rwm.h"
#include "common/tl/parse.h"
#include "common/tl/query-header.h"
#include "net/net-buffers.h"
#include "net/net-connections.h"
#include "net/net-crypto-aes.h"
#include "net/net-dc.h"
#include "net/net-http-server.h"
#include "net/net-ifnet.h"
#include "net/net-memcache-client.h"
#include "net/net-memcache-server.h"
#include "net/net-mysql-client.h"
#include "net/net-sockaddr-storage.h"
#include "net/net-socket.h"
#include "net/net-tcp-connections.h"
#include "net/net-tcp-rpc-client.h"
#include "net/net-tcp-rpc-server.h"

#include "runtime/interface.h"
#include "runtime/profiler.h"
#include "runtime/job-workers/shared-memory-manager.h"
#include "runtime/rpc.h"
#include "server/confdata-binlog-replay.h"
#include "server/job-workers/job-worker-client.h"
#include "server/job-workers/job-worker-server.h"
#include "server/job-workers/job-workers-context.h"
#include "server/json-logger.h"
#include "server/lease-config-parser.h"
#include "server/php-engine-vars.h"
#include "server/php-lease.h"
#include "server/php-master-warmup.h"
#include "server/php-master.h"
#include "server/php-mc-connections.h"
#include "server/php-queries.h"
#include "server/php-runner.h"
#include "server/php-sql-connections.h"
#include "server/php-worker-stats.h"
#include "server/php-worker.h"

using job_workers::JobWorkersContext;
using job_workers::JobWorkerClient;
using job_workers::JobWorkerServer;


#define MAX_TIMEOUT 30

#define RPC_DEFAULT_PING_INTERVAL 10.0
double rpc_ping_interval = RPC_DEFAULT_PING_INTERVAL;
#define RPC_STOP_INTERVAL (2 * rpc_ping_interval)
#define RPC_FAIL_INTERVAL (3 * rpc_ping_interval)

#define RPC_CONNECT_TIMEOUT 3


/***
  RPC-client
 ***/
int rpcc_send_query(connection *c);
int rpcc_check_ready(connection *c);
int rpcx_execute(connection *c, int op, raw_message *raw);

// Receives engine responses
tcp_rpc_client_functions tcp_rpc_client_outbound = [] {
  auto res = tcp_rpc_client_functions();
  res.execute = rpcx_execute; //replaced
  res.check_ready = rpcc_check_ready; //replaced
  res.flush_packet = tcp_rpcc_flush_packet_later;
  res.rpc_check_perm = tcp_rpcc_default_check_perm;
  res.rpc_init_crypto = tcp_rpcc_init_crypto;
  res.rpc_start_crypto = tcp_rpcc_start_crypto;
  res.rpc_ready = rpcc_send_query; //replaced

  return res;
}();

/***
  OUTBOUND CONNECTIONS
 ***/

static int db_port = 3306;

conn_type_t ct_tcp_rpc_client_read_all = [] {
  auto res = get_default_tcp_rpc_client_conn_type();
  res.reader = tcp_server_reader_till_end;
  return res;
}();

// Engines communication target (rpc-responses)
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
  ct->endpoint = make_inet_sockaddr_storage(ip, static_cast<uint16_t>(port));

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

  ct->endpoint = make_inet_sockaddr_storage(ntohl(*(uint32_t *)h->h_addr), static_cast<uint16_t>(port));

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

int delete_pending_query(conn_query *q) {
  vkprintf (1, "delete_pending_query(%p,%p)\n", q, q->requester);

  delete_conn_query(q);
  free(q);
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


/** net write command **/
/** do write delayed write to connection **/



void command_net_write_run_rpc(command_t *base_command, void *data) {
  auto *command = (command_net_write_t *)base_command;
//  fprintf (stderr, "command_net_write [ptr=%p] [len = %d] [data = %p]\n", base_command, command->len, data);

  slot_id_t slot_id = static_cast<slot_id_t>(command->extra);
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

int has_pending_scripts() {
  return php_worker_run_flag || pending_http_queue.first_query != (conn_query *)&pending_http_queue;
}

void net_error(net_ansgen_t *ansgen, php_query_base_t *query, const char *err) {
  ansgen->func->error(ansgen, err);
  query->ans = ansgen->ans;
  ansgen->func->free(ansgen);
  php_script_query_answered(php_script);
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
  tcp_rpc_conn_send_data(c, static_cast<int>(qsize - 3 * sizeof(int)), q + 2);

  TCP_RPCS_FUNC(c)->flush_packet(c);
}

void on_net_event(int event_status) {
  if (event_status == 0) {
    return;
  }
  assert (active_worker != nullptr);
  if (event_status < 0) {
    php_worker_terminate(active_worker, 0, script_error_t::net_event_error, "memory limit on net event");
    php_worker_wakeup(active_worker);
    return;
  }
  if (active_worker->waiting) {
    php_worker_wakeup(active_worker);
  }
}

static void send_rpc_error_raw(connection *c, long long req_id, int code, const char *str) {
  static int q[10000];
  q[2] = TL_RPC_REQ_ERROR;
  *(long long *)(q + 3) = req_id;
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
}

void server_rpc_error(connection *c, long long req_id, int code, const char *str) {
  send_rpc_error_raw(c, req_id, code, str);
  TCP_RPCS_FUNC(c)->flush_packet(c);
}

void client_rpc_error(connection *c, long long req_id, int code, const char *str) {
  send_rpc_error_raw(c, req_id, code, str);
  TCP_RPCC_FUNC(c)->flush_packet(c);
}

int hts_php_wakeup(connection *c);

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

  // TODO drop it?
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
  php_worker *worker = php_worker_create(http_worker, c, http_data, nullptr, nullptr, ++http_script_req_id, script_timeout);
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
    php_worker_terminate(worker, 1, script_error_t::http_connection_close, "http connection close");
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

// For tasks engine, rpc-proxy task-related communication and RPC microservice requests processing
tcp_rpc_client_functions rpc_client_methods = [] {
  auto res = tcp_rpc_client_functions();
  res.execute = rpcx_execute; //replaced
  res.check_ready = default_tcp_rpc_client_check_ready; //replaced
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

// Tasks engine communication target
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

  auto target_fd = c->target - Targets;
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
    php_worker_terminate(worker, 1, script_error_t::rpc_connection_close, "rpc connection close");
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

static bool check_tasks_invoker_pid(process_id_t tasks_invoker_pid) {
  if (tasks_invoker_pid.ip == 0) {
    tasks_invoker_pid.ip = LOCALHOST;
  }
  process_id_t lease_pid = get_lease_pid();
  return matches_pid(&tasks_invoker_pid, &lease_pid) != no_pid_match;
}

static bool check_tasks_manager_pid(process_id_t tasks_manager_pid) {
  if (tasks_manager_pid.ip == 0) {
    tasks_manager_pid.ip = LOCALHOST;
  }
  process_id_t rpc_main_target_pid = get_rpc_main_target_pid();
  return matches_pid(&tasks_manager_pid, &rpc_main_target_pid) != no_pid_match;
}

static double normalize_script_timeout(double timeout_sec) {
  if (timeout_sec < 1) {
    kprintf("Too small script timeout: %f sec, should be [%d..%d] sec", timeout_sec, 1, MAX_SCRIPT_TIMEOUT);
    return 1;
  }
  if (timeout_sec > MAX_SCRIPT_TIMEOUT) {
    kprintf("Too big script timeout: %f sec, should be [%d..%d] sec", timeout_sec, 1, MAX_SCRIPT_TIMEOUT);
    return MAX_SCRIPT_TIMEOUT;
  }
  return timeout_sec;
}

static void send_rpc_error(connection *c, long long req_id, int error_code, const char *error_msg) {
  if (c->type == &ct_php_engine_rpc_server) {
    server_rpc_error(c, req_id, error_code, error_msg);
  } else {
    client_rpc_error(c, req_id, error_code, error_msg);
  }
}

int rpcx_execute(connection *c, int op, raw_message *raw) {
  vkprintf(1, "rpcx_execute: fd=%d, op=%d, len=%d\n", c->fd, op, raw->total_bytes);

  int len = raw->total_bytes;

  bool in_sigterm = sigterm_on && sigterm_time < precise_now;
  c->last_response_time = precise_now;

  auto MAX_RPC_QUERY_LEN = 126214400;
  if (len < sizeof(long long) || MAX_RPC_QUERY_LEN < len) {
    return 0;
  }

  int event_status = 0;
  process_id remote_pid = TCP_RPC_DATA(c)->remote_pid;

  switch (static_cast<unsigned int>(op)) {
    case TL_KPHP_STOP_LEASE: {
      if (in_sigterm) {
        return 0;
      }
      do_rpc_stop_lease();
      break;
    }
    case TL_KPHP_START_LEASE_V2:
    case TL_KPHP_START_LEASE: {
      // rpc-proxy response with task PID
      if (!check_tasks_manager_pid(remote_pid) || in_sigterm) {
        return 0;
      }
      tl_fetch_init_raw_message(raw);
      auto op_from_tl = tl_fetch_int();
      len -= static_cast<int>(sizeof(op_from_tl));
      assert(op_from_tl == op);
      assert(len % sizeof(int) == 0);

      static_assert(sizeof(process_id_t) == 12, "");

      size_t expected_min_size = (op == TL_KPHP_START_LEASE_V2 ? sizeof(int) + sizeof(process_id_t) + sizeof(int) : sizeof(process_id_t) + sizeof(int));
      if (len < expected_min_size) {
        return 0;
      }

      int fields_mask = 0;

      if (op == TL_KPHP_START_LEASE_V2) {
        fields_mask = tl_fetch_int();
      }

      union {
        std::array<char, sizeof(process_id_t)> buf;
        process_id_t xpid;
      };
      auto fetched_bytes = tl_fetch_data(buf.data(), static_cast<int>(buf.size()));
      assert(fetched_bytes == buf.size());

      int timeout = tl_fetch_int();

      int actor_id = -1;

      if (fields_mask & vk::tl::kphp::start_lease_v2_fields_mask::actor_id) {
        actor_id = tl_fetch_int();
      }

      // get tasks target by the PID and send kphp.readyV2 to the target (in other words, do a task request)
      do_rpc_start_lease(xpid, precise_now + timeout, actor_id);
      return 0;
    }
    case TL_RPC_INVOKE_REQ: {
      if (in_sigterm) {
        return 0;
      }
      // got a new task from the tasks engine or a request from RPC microservice client
      tl_fetch_init_raw_message(raw);

      int64_t left_bytes_with_headers = tl_fetch_unread();
      tl_query_header_t header{};
      bool header_fetched_successfully = tl_fetch_query_header(&header);
      if (!header_fetched_successfully) {
        send_rpc_error(c, header.qid, tl_fetch_error_code(), tl_fetch_error_string());
        return 0;
      }
      int64_t left_bytes_without_headers = tl_fetch_unread();

      len -= (left_bytes_with_headers - left_bytes_without_headers);
      assert(len % 4 == 0);

      long long req_id = header.qid;

      vkprintf(2, "got RPC_INVOKE_REQ [req_id = %016llx]\n", req_id);

      if (!check_tasks_invoker_pid(remote_pid)) {
        static char msg_buf[1000];
        process_id_t lease_pid = get_lease_pid();
        sprintf(msg_buf, "Task invoker is invalid. Expected %s, but actual %s", pid_to_print(&lease_pid), pid_to_print(&remote_pid));
        send_rpc_error(c, req_id, TL_ERROR_QUERY_INCORRECT, msg_buf);
        return 0;
      }
      if (c->type != &ct_php_rpc_client && c->type != &ct_php_engine_rpc_server) {
        connection *lease_connection = get_lease_connection();
        if (lease_connection == nullptr || lease_connection->status != conn_expect_query) {
          client_rpc_error(c, req_id, TL_ERROR_QUERY_INCORRECT, "Task connection is busy");
          return 0;
        }
        c = lease_connection;
      }
      auto custom_settings = try_fetch_lookup_custom_worker_settings();
      double actual_script_timeout = custom_settings.has_timeout() ? normalize_script_timeout(custom_settings.php_timeout_ms / 1000.0) : script_timeout;
      set_connection_timeout(c, actual_script_timeout);

      char buf[len + 1];
      auto fetched_bytes = tl_fetch_data(buf, len);
      if (fetched_bytes == -1) {
        client_rpc_error(c, req_id, tl_fetch_error_code(), tl_fetch_error_string());
        return 0;
      }
      assert(fetched_bytes == len);
      auto D = TCP_RPC_DATA(c);
      rpc_query_data *rpc_data = rpc_query_data_create(std::move(header), reinterpret_cast<int *>(buf), len / static_cast<int>(sizeof(int)), D->remote_pid.ip,
                                                       D->remote_pid.port, D->remote_pid.pid, D->remote_pid.utime);

      php_worker *worker = php_worker_create(run_once ? once_worker : rpc_worker, c, nullptr, rpc_data, nullptr, req_id, actual_script_timeout);
      D->extra = worker;

      c->status = conn_wait_net;
      rpcx_func_wakeup(c);
      break;
    }
    case TL_RPC_REQ_ERROR:
    case TL_RPC_REQ_RESULT: {
      // received either error or result from an engine
      int result_len = raw->total_bytes - static_cast<int>(sizeof(int) + sizeof(long long));
      assert(result_len >= 0);

      tl_fetch_init_raw_message(raw);

      auto op_from_tl = tl_fetch_int();
      assert(op_from_tl == op);

      auto id = tl_fetch_long();
      if (op == TL_RPC_REQ_ERROR) {
        //FIXME: error code, error string
        //almost never happens
        event_status = create_rpc_error_event(static_cast<slot_id_t>(id), -1, "unknown error", nullptr);
        break;
      }

      net_event_t *event = nullptr;
      event_status = create_rpc_answer_event(static_cast<slot_id_t>(id), result_len, &event);
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
  free(q);
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
  free(q);
  return 0;
}

/***
  MEMCACHED CLIENT
 ***/

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


void create_pnet_delayed_query(connection *http_conn, conn_target_t *t, net_ansgen_t *gen, double finish_time) {
  auto q = reinterpret_cast<conn_query *>(malloc(sizeof(conn_query)));

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
  auto q = reinterpret_cast<conn_query *>(malloc(sizeof(conn_query)));

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
  auto q = reinterpret_cast<conn_query *>(malloc(sizeof(conn_query)));

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
    free(q);
  }
  return 0;
}

static char stats[65536];
static int stats_len;

void prepare_full_stats() {
  char *s = stats;
  int s_left = 65530;

  PhpWorkerStats::get_local().update_idle_time(epoll_total_idle_time(), get_uptime(),
                                               epoll_average_idle_time(), epoll_average_idle_quotient());
  PhpWorkerStats::get_local().recalc_worker_percentiles();
  const int stats_size = PhpWorkerStats::get_local().write_into(s, s_left);
  s += stats_size;
  s_left -= stats_size;

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

  pending_signals = pending_signals | (1 << sig);
  ksignal(sig, sigint_immediate_handler);
}

void turn_sigterm_on() {
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

  pending_signals = pending_signals | (1ll << sig);
}

static void sigusr1_handler(const int sig) {
  const char message[] = "got SIGUSR1, rotate logs.\n";
  kwrite(2, message, sizeof(message) - (size_t)1);

  pending_signals = pending_signals | (1ll << sig);
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
#if defined(__APPLE__)
  siginfo_t signal_info_stub;
  signal_info_stub.si_code = SI_QUEUE;
  signal_info_stub.si_value.sival_int =  SPOLL_SEND_STATS | SPOLL_SEND_FULL_STATS | SPOLL_SEND_IMMEDIATE_STATS;
  info = &signal_info_stub;
#endif
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

void reopen_json_log() {
  if (logname) {
    char worker_json_log_file_name[PATH_MAX];
    sprintf(worker_json_log_file_name, "%s.json", logname);
    if (!vk::singleton<JsonLogger>::get().reopen_log_file(worker_json_log_file_name)) {
      vkprintf(-1, "failed to open log '%s': error=%s", worker_json_log_file_name, strerror(errno));
    }
  }
}

static void generic_event_loop(RunMode mode) {
  if (master_flag && logname_pattern != nullptr) {
    reopen_logs();
    reopen_json_log();
  }

  int prev_time = 0;
  double next_create_outbound = 0;

  switch (mode) {
    case RunMode::master: {
      if (rpc_port > 0 && rpc_sfd < 0) {
        rpc_sfd = server_socket(rpc_port, settings_addr, backlog, 0);
        if (rpc_sfd < 0) {
          vkprintf (-1, "cannot open rpc server socket at port %d: %m\n", rpc_port);
          exit(1);
        }
      }

      if (rpc_sfd >= 0) {
        init_listening_connection(rpc_sfd, &ct_php_engine_rpc_server, &rpc_methods);
      }

      if (run_once) {
        int pipe_fd[2];
        pipe(pipe_fd);

        int read_fd = pipe_fd[0];
        int write_fd = pipe_fd[1];

        rpc_client_methods.rpc_ready = nullptr;
        epoll_insert_pipe(pipe_for_read, read_fd, &ct_php_rpc_client, &rpc_client_methods);

        int q[6];
        int qsize = 6 * sizeof(int);
        q[2] = TL_RPC_INVOKE_REQ;
        for (int i = 0; i < run_once_count; i++) {
          prepare_rpc_query_raw(i, q, qsize, crc32c_partial);
          assert (write(write_fd, q, (size_t)qsize) == qsize);
        }
      }
    }
    // fall through
    case RunMode::http_worker: {
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

      if (verbosity > 0 && http_sfd >= 0) {
        vkprintf (-1, "created listening socket at %s:%d, fd=%d\n", ip_to_print(settings_addr.s_addr), http_port, http_sfd);
      }

      if (http_sfd >= 0) {
        init_listening_tcpv6_connection(http_sfd, &ct_php_engine_http_server, &http_methods, SM_SPECIAL);
      }

      auto &rpc_clients = RpcClients::get().rpc_clients;
      std::for_each(rpc_clients.begin(), rpc_clients.end(),[](LeaseRpcClient &rpc_client) {
        vkprintf(-1, "create rpc client target: %s:%d\n", rpc_client.host.c_str(), rpc_client.port);
        rpc_client.target_id = get_target(rpc_client.host.c_str(), rpc_client.port, &rpc_client_ct);
      });
      if (!rpc_clients.empty()) {
        set_main_target(rpc_clients.front());
      }

      if (vk::singleton<JobWorkersContext>::get().job_workers_num > 0) {
        vk::singleton<JobWorkerClient>::get().init(logname_id);
      }
      break;
    }
    case RunMode::job_worker: {
      vk::singleton<JobWorkerServer>::get().init();
      break;
    }
  }

  if (no_sql) {
    sql_target_id = -1;
  } else {
    sql_target_id = get_target("localhost", db_port, &db_ct);
    assert (sql_target_id != -1);
  }

  get_utime_monotonic();
  //create_all_outbound_connections();

  ksignal(SIGTERM, sigterm_handler);
  ksignal(SIGPIPE, SIG_IGN);
  ksignal(SIGINT, run_once ? sigint_immediate_handler : sigint_handler);
  ksignal(SIGUSR1, sigusr1_handler);
  ksignal(SIGPOLL, SIG_IGN);

  //using sigaction for SIGSTAT
  dl_sigaction(SIGSTAT, nullptr, dl_get_empty_sigset(), SA_SIGINFO | SA_ONSTACK | SA_RESTART, sigstats_handler);

  dl_allow_all_signals();

  auto &job_worker_server = vk::singleton<JobWorkerServer>::get();

  vkprintf (1, "Server started\n");
  for (int i = 0; !(pending_signals & ~((1ll << SIGUSR1) | (1ll << SIGHUP))); i++) {
    if (verbosity > 0 && !(i & 255)) {
      vkprintf (1, "epoll_work(): %d out of %d connections, network buffers: %d used, %d out of %d allocated\n",
                active_connections, maxconn, NB_used, NB_alloc, NB_max);
    }
    if (mode == RunMode::job_worker) {
      job_worker_server.try_complete_delayed_jobs();
    }
    epoll_work(57);

    if (precise_now > next_create_outbound) {
      create_all_outbound_connections();
      next_create_outbound = precise_now + 0.03 + 0.02 * drand48();
    }

    while (spoll_send_stats > 0) {
      write_full_stats_to_pipe();
      spoll_send_stats--;
      if (mode == RunMode::job_worker) {
        job_worker_server.last_stats.start();
      }
    }

    if (pending_signals & (1ll << SIGHUP)) {
      pending_signals = pending_signals & ~(1ll << SIGHUP);
    }

    if (pending_signals & (1ll << SIGUSR1)) {
      pending_signals = pending_signals & ~(1ll << SIGUSR1);

      reopen_logs();
      reopen_json_log();
    }

    if (now != prev_time) {
      prev_time = now;
      cron();
    }

    if (vk::any_of_equal(mode, RunMode::http_worker, RunMode::master)) {
      lease_cron();
      if (sigterm_on && !rpc_stopped) {
        rpcc_stop();
      }
      if (sigterm_on && !hts_stopped) {
        hts_stop();
      }
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

  if (mode == RunMode::http_worker && http_sfd >= 0) {
    epoll_close(http_sfd);
    assert (close(http_sfd) >= 0);
  }
}

void start_server() {
  pending_signals = 0;
  if (daemonize) {
    setsid();

    ksignal(SIGHUP, sighup_handler);
    reopen_logs();
    reopen_json_log();
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
  } else {
    run_mode = RunMode::master;
  }

  generic_event_loop(run_mode);
}

void set_instance_cache_memory_limit(size_t limit);
void init_php_scripts() noexcept;
void global_init_php_scripts() noexcept;
const char *get_php_scripts_version() noexcept;
char **get_runtime_options(int *count) noexcept;


void init_all() {
  srand48((long)cycleclock_now());

  //init pending_http_queue
  pending_http_queue.first_query = pending_http_queue.last_query = (conn_query *)&pending_http_queue;
  php_worker_run_flag = 0;

  dummy_request_queue.first_query = dummy_request_queue.last_query = (conn_query *)&dummy_request_queue;

  if (script_timeout == 0) {
    script_timeout = run_once ? 1e6 : DEFAULT_SCRIPT_TIMEOUT;
  }
  const auto *tag = engine_tag ?: "0";
  vk::singleton<JsonLogger>::get().init(string::to_int(tag, static_cast<string::size_type>(strlen(tag))), script_timeout);

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

  init_confdata_binlog_reader();
}

void init_logname(const char *src) {
  if (!std::strchr(src, '%')) {
    logname = src;
    kprintf_multiprocessing_mode_enable();
    return;
  }

  char pattern_buf[100];
  char plane_buf[100];
  int buf_len = 100;

  char *pattern = pattern_buf;
  char *plane = plane_buf;

  bool was_percent = false;
  while (*src) {
    assert (pattern < pattern_buf + buf_len - 3);
    if (*src == '%') {
      if (!was_percent) {
        *pattern++ = '%';
        *pattern++ = 'd';
        was_percent = true;
      }
    } else {
      *pattern++ = *src;
      // ignore the first '-%' so we get 'engine-kp-%.log' -> 'engine-kp.log'
      const bool skip_dash = *src == '-' && *(src + 1) == '%' && !was_percent;
      if (!skip_dash) {
        *plane++ = *src;
      }
    }
    src++;
  }
  *pattern = 0;
  *plane = 0;

  logname = strdup(plane_buf);
  logname_pattern = strdup(pattern_buf);
}

static double parse_double_option(const char *option_name, const double min_value, const double max_value) {
  double result{};
  try {
    result = std::stod(optarg);
  } catch (const std::exception &e) {
    kprintf("%s option parse error: %s", option_name, e.what());
    return std::numeric_limits<double>::quiet_NaN();
  }
  if (min_value <= result && result <= max_value) {
    return result;
  }
  kprintf("%s should be in [%f; %f]", option_name, min_value, max_value);
  return std::numeric_limits<double>::quiet_NaN();
}

/** main arguments parsing **/
int main_args_handler(int i) {
  switch (i) {
    case 'D': {
      char *key = optarg, *value;
      char *eq = strchr(key, '=');
      if (eq == nullptr) {
        kprintf("-D option, can't find '='\n");
        return -1;
      }
      value = eq + 1;
      *eq = 0;
      ini_set(key, value);
      *eq = '=';
      return 0;
    }
    case 'i': {
      const char *config_file_name = optarg;
      if (int failed_line = ini_set_from_config(config_file_name)) {
        if (failed_line == -1) {
          kprintf("-i option, can't open ini_get config file: %s\n", config_file_name);
        } else {
          kprintf("-i option, can't find '=' (line %d at file %s)\n", failed_line, config_file_name);
        }
        return -1;
      }
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
        kprintf("-w option, can't find ':'\n");
        return -1;
      }
      LeaseRpcClient rpc_client;
      rpc_client.host.assign(optarg, colon - optarg);
      char *at = strchr(colon, '@');
      if (at) {
        *at = 0;
      }
      rpc_client.port = atoi(colon + 1);
      if (at) {
        *at = '@';
        rpc_client.actor = atoi(at + 1);
      }
      if (const char *err = rpc_client.check()) {
        kprintf("-w option: %s\n", err);
        return -1;
      }
      RpcClients::get().rpc_clients.push_back(std::move(rpc_client));
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
    case 't': {
      script_timeout = static_cast<int>(normalize_script_timeout(atoi(optarg)));
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
        kprintf("error: fail to lock paged memory\n");
      }
      return 0;
    }
    case 'S': {
      const char *lease_config = optarg;
      return LeaseConfigParser::parse_lease_options_config(lease_config);
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
      if (instance_cache_memory_limit <= 0 || instance_cache_memory_limit > memory_resource::memory_buffer_limit()) {
        kprintf("couldn't parse instance-cache-memory-limit argument\n");
        return -1;
      }
      set_instance_cache_memory_limit(static_cast<size_t>(instance_cache_memory_limit));
      return 0;
    }
    case 2004: {
      set_confdata_binlog_mask(optarg);
      return 0;
    }
    case 2005: {
      int64_t confdata_memory_limit = parse_memory_limit(optarg);
      if (confdata_memory_limit <= 0 || confdata_memory_limit > memory_resource::memory_buffer_limit()) {
        kprintf("couldn't parse confdata-memory-limit argument\n");
        return -1;
      }
      set_confdata_memory_limit(static_cast<size_t>(confdata_memory_limit));
      return 0;
    }
    case 2006: {
      if (!*optarg) {
        set_confdata_blacklist_pattern(nullptr);
        return 0;
      }
      auto re2_pattern = std::make_unique<re2::RE2>(optarg, re2::RE2::Quiet);
      if (!re2_pattern->ok()) {
        kprintf("couldn't parse confdata-blacklist pattern: %s\n", re2_pattern->error().c_str());
        return -1;
      }
      set_confdata_blacklist_pattern(std::move(re2_pattern));
      return 0;
    }
    case 2007: {
      if (!*optarg) {
        clear_confdata_predefined_wildcards();
      } else {
        add_confdata_predefined_wildcard(optarg);
      }
      return 0;
    }
    case 2008: {
      printf("%s\n", get_php_scripts_version());
      exit(0);
    }
    case 2009: {
      php_warning_minimum_level = atoi(optarg);
      if (php_warning_minimum_level < 0 || php_warning_minimum_level > 3) {
        kprintf("--php-warnings-minimal-verbosity has to be [0, 3]\n");
        return -1;
      }
      return 0;
    }
    case 2010: {
      if (set_profiler_log_path(optarg)) {
        return 0;
      }
      kprintf("couldn't set profiler-log-prefix '%s'\n", optarg);
      return -1;
    }
    case 2011: {
      if (set_mysql_db_name(optarg)) {
        return 0;
      }
      kprintf("couldn't set mysql-db-name '%s'\n", optarg);
      return -1;
    }
    case 2012: {
      if (add_dc_by_ipv4_config(optarg)) {    // can appear multiple times, each adding a new dc config
        return 0;
      }
      kprintf("couldn't set net-dc-mask '%s'\n", optarg);
      return -1;
    }
    case 2013: {
      double warmup_workers_part = parse_double_option("--warmup-workers-ratio", 0, 1);
      if (std::isnan(warmup_workers_part)) {
        return -1;
      }
      WarmUpContext::get().set_workers_part_for_warm_up(warmup_workers_part);
      return 0;
    }
    case 2014: {
      double warmup_instance_cache_elements_part = parse_double_option("--warmup-instance-cache-elements-ratio", 0, 1);
      if (std::isnan(warmup_instance_cache_elements_part)) {
        return -1;
      }
      WarmUpContext::get().set_target_instance_cache_elements_part(warmup_instance_cache_elements_part);
      return 0;
    }
    case 2015: {
      double warmup_timeout_sec = parse_double_option("--warmup-timeout", 0, DEFAULT_SCRIPT_TIMEOUT);
      if (std::isnan(warmup_timeout_sec)) {
        return -1;
      }
      WarmUpContext::get().set_warm_up_max_time(std::chrono::duration<double>{warmup_timeout_sec});
      return 0;
    }
    case 2016: {
      // TODO: add check
      vk::singleton<JobWorkersContext>::get().job_workers_num = vk::clamp(atoi(optarg), 0, MAX_WORKERS / 3);
      return 0;
    }
    case 2017: {
      // TODO: add check
      size_t mbs = atoi(optarg);
      vk::singleton<job_workers::SharedMemoryManager>::get().set_memory_limit(mbs * 1024 * 1024);
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

DEPRECATED_OPTION("use-unix", no_argument);
DEPRECATED_OPTION_SHORT("json-log", 'j', no_argument);
DEPRECATED_OPTION_SHORT("crc32c", 'C', no_argument);
DEPRECATED_OPTION_SHORT("tl-schema", 'T', required_argument);

void parse_main_args(int argc, char *argv[]) {
  usage_set_other_args_desc("");
  option_section_t sections[] = {OPT_GENERIC, OPT_NETWORK, OPT_RPC, OPT_VERBOSITY, OPT_ENGINE_CUSTOM, OPT_DEPRECATED, OPT_ARRAY_END};
  init_parse_options(sections);

  parse_option("log", required_argument, 'l', "set log name. %% can be used for log-file per worker");
  parse_option("lock-memory", no_argument, 'k', "lock paged memory");
  parse_option("define", required_argument, 'D', "set data for ini_get (in form key=value)");
  parse_option("define-from-config", required_argument, 'i', "set data for ini_get from config file (in form key=value on each row)");
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
  parse_option("time-limit", required_argument, 't', "time limit for script in seconds");
  parse_option("small-acsess-log", optional_argument, 'U', "don't write get data in log. If used twice (or with value 2), disables access log.");
  parse_option("fatal-warnings", no_argument, 'K', "script is killed, when warning happened");
  parse_option("worker-queries-to-reload", required_argument, 2000, "worker script is reloaded, when <queries> queries processed (default: 100)");
  parse_option("worker-memory-to-reload", required_argument, 2001, "worker script is reloaded, when <memory> queries processed");
  parse_option("use-madvise-dontneed", no_argument, 2002, "Use madvise MADV_DONTNEED for script memory above limit");
  parse_option("instance-cache-memory-limit", required_argument, 2003, "memory limit for instance_cache");
  parse_option("tasks-config", required_argument, 'S', "get lease worker settings from config file: mode and actor");
  parse_option("confdata-binlog", required_argument, 2004, "confdata binlog mask");
  parse_option("confdata-memory-limit", required_argument, 2005, "memory limit for confdata");
  parse_option("confdata-blacklist", required_argument, 2006, "confdata key blacklist regex pattern");
  parse_option("confdata-predefined-wildcard", required_argument, 2007, "perdefine confdata wildcard for better performance");
  parse_option("php-version", no_argument, 2008, "show the compiled php code version and exit");
  parse_option("php-warnings-minimal-verbosity", required_argument, 2009, "set minimum verbosity level for php warnings");
  parse_option("profiler-log-prefix", required_argument, 2010, "set profier log path perfix");
  parse_option("mysql-db-name", required_argument, 2011, "database name of MySQL to connect");
  parse_option("net-dc-mask", required_argument, 2012, "a string formatted like '8=1.2.3.4/12' to detect a datacenter by ipv4");
  parse_option("warmup-workers-ratio", required_argument, 2013, "the ratio of the instance cache warming up workers during the graceful restart");
  parse_option("warmup-instance-cache-elements-ratio", required_argument, 2014, "the ratio of the instance cache elements which makes the instance cache hot enough");
  parse_option("warmup-timeout", required_argument, 2015, "the maximum time for the instance cache warm up in seconds");
  parse_option("job-workers-num", required_argument, 2016, "number of job workers to run");
  parse_option("job-workers-shared-memory-size", required_argument, 2017, "total size of shared memory in MBs used for job workers related communication");
  parse_engine_options_long(argc, argv, main_args_handler);
  parse_main_args_till_option(argc, argv);
}


void init_default() {
  dl_set_default_handlers();
  now = (int)time(nullptr);

  pid = getpid();
  // RPC part
  PID.port = (short)rpc_port;

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

  if (!master_flag && !daemonize) {
    kstdout = dup(1);
    if (kstdout <= 2) {
      kprintf("fatal: can't save stdout\n");
      exit(1);
    }
    kstderr = dup(2);
    if (kstderr <= 2) {
      kprintf("fatal: can't save stderr\n");
      exit(1);
    }
  }

  if (logname) {
    reopen_logs();
    reopen_json_log();
  }
}

int run_main(int argc, char **argv, php_mode mode) {
  init_version_string(NAME_VERSION);
  dl_block_all_signals();
#if !ASAN_ENABLED
  set_core_dump_rlimit(1LL << 40);
#endif
  max_special_connections = 1;
  static_assert(offsetof(tcp_rpc_client_functions, rpc_ready) == offsetof(tcp_rpc_server_functions, rpc_ready), "");

  if (mode == php_mode::cli) {
    run_once = 1;
    disable_access_log = 2;
    parse_main_args_till_option(argc, argv, "--Xkphp-options");
  }

  std::vector<char *> new_argv;
  int options_count = 0;
  if (char **options_from_php = get_runtime_options(&options_count)) {
    const int parsed_options = std::max(optind, 1);
    new_argv.assign(argv, argv + parsed_options);
    std::copy(options_from_php, options_from_php + options_count, std::back_inserter(new_argv));
    std::copy(argv + parsed_options, argv + argc, std::back_inserter(new_argv));
    argv = new_argv.data();
    argc = static_cast<int>(new_argv.size());
  }

  parse_main_args(argc, argv);

  size_t total_workers = workers_n + vk::singleton<JobWorkersContext>::get().job_workers_num;
  if (total_workers > MAX_WORKERS) {
    kprintf("Too many workers: %zu, maximum is %d\n", total_workers, MAX_WORKERS);
    exit(1);
  }

  if (run_once) {
    master_flag = 0;
    rpc_port = -1;
    http_port = -1;
    setvbuf(stdout, nullptr, _IONBF, 0);
  }

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
