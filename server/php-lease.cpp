// Compiler for PHP (aka KPHP)
// Copyright (c) 2021 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "server/php-lease.h"

#include "common/kprintf.h"
#include "common/options.h"
#include "common/precise-time.h"
#include "common/timer.h"
#include "common/tl/constants/common.h"
#include "common/tl/constants/kphp.h"
#include "common/tl/methods/string.h"
#include "common/tl/parse.h"
#include "common/tl/store.h"

#include "net/net-connections.h"
#include "net/net-sockaddr-storage.h"
#include "net/net-tcp-rpc-common.h"

#include "server/lease-context.h"
#include "server/php-engine-vars.h"
#include "server/php-worker.h"

DEFINE_VERBOSITY(lease);

namespace {

int rpc_proxy_target = -1;

int cur_lease_target_ip = -1;
int cur_lease_target_port = -1;
conn_target_t *cur_lease_target_ct = nullptr;
int cur_lease_target = -1;

int rpc_lease_target = -1;
double rpc_lease_timeout = -1;
process_id_t lease_pid;
int lease_actor_id = -1;

double lease_stats_start_time;
double lease_stats_time;
long long lease_stats_cnt;
int ready_cnt = 0;

bool stop_ready_ack_received = false;
vk::SteadyTimer<std::chrono::microseconds> stop_ready_ack_timer;

bool is_stop_ready_timeout_expired() {
  const std::chrono::duration<double> rpc_stop_ready_timeout{vk::singleton<LeaseContext>::get().rpc_stop_ready_timeout};
  return stop_ready_ack_timer.time() > rpc_stop_ready_timeout;
}

enum class lease_state_t {
  off,   // connect to rpc-proxy, wait kphp.startLease from it, connect to target
  start, // if !has_pending_scripts -> change state to lease_state_t::on, wait for connection to target ready, do lease_set_ready() && run_rpc_lease();
  on,    // connecting to target, send RPC_READY, doing work, if too much time doing work -> change state to lease_state_t::initiating_finish
  initiating_finish, // wait finishing current task, send RPC_STOP to target, send TL_KPHP_LEASE_STATS to rpc-proxy, change state to lease_state_t::finish
  finish,            // wait TL_KPHP_STOP_READY_ACKNOWLEDGMENT from target with 1 sec timeout, then change state to lease_state_t::off
};

const char *lease_state_to_str[] = {
  "off", "start", "on", "initiating_finish", "finish",
};

lease_state_t lease_state = lease_state_t::off;
bool lease_ready_flag = false; // waiting for something -> equal false

} // namespace

extern conn_target_t rpc_client_ct;

int get_target_by_pid(int ip, int port, conn_target_t *ct);
void send_rpc_query(connection *c, int op, long long id, int *q, int qsize);
connection *get_target_connection(conn_target_t *S, int force_flag);
int has_pending_scripts();

static int get_lease_target_by_pid(int ip, int port, conn_target_t *ct) {
  if (ip == cur_lease_target_ip && port == cur_lease_target_port && ct == cur_lease_target_ct) {
    return cur_lease_target;
  }
  if (cur_lease_target != -1) {
    conn_target_t *old_target = &Targets[cur_lease_target];
    destroy_target(old_target);
  }
  cur_lease_target_ip = ip;
  cur_lease_target_port = port;
  cur_lease_target_ct = ct;
  cur_lease_target = get_target_by_pid(ip, port, ct);

  return cur_lease_target;
}

static void lease_change_state(lease_state_t new_state) {
  if (lease_state != new_state) {
    tvkprintf(lease, 2, "Change lease state: %s -> %s\n", lease_state_to_str[static_cast<int>(lease_state)], lease_state_to_str[static_cast<int>(new_state)]);
    lease_state = new_state;
    lease_ready_flag = false;
  }
}

void lease_on_worker_finish(PhpWorker *worker) {
  if ((lease_state == lease_state_t::on || lease_state == lease_state_t::initiating_finish) && worker->target_fd == rpc_lease_target) {
    double worked = precise_now - worker->start_time;
    lease_stats_time += worked;
    lease_stats_cnt++;
  }
}

static int wrap_rpc_dest_actor_raw(int *raw_rpc_query_buf, int actor_id, int magic) {
  int pos = 0;
  *reinterpret_cast<long long *>(&raw_rpc_query_buf[pos]) = 0;
  pos += 2;
  raw_rpc_query_buf[pos++] = TL_RPC_DEST_ACTOR;
  *reinterpret_cast<long long *>(&raw_rpc_query_buf[pos]) = actor_id;
  pos += 2;
  raw_rpc_query_buf[pos++] = magic;
  return pos;
}

static void rpc_send_stopped(connection *c) {
  static constexpr int QUERY_INT_BUF_LEN = 1000;
  static int q[QUERY_INT_BUF_LEN];
  int qn = 0;
  qn += 2;

  int magic = TL_KPHP_STOP_READY;
  q[qn++] = -1;

  int actor_id = lease_actor_id;
  if (actor_id != -1) {
    qn += wrap_rpc_dest_actor_raw(q + qn, actor_id, magic);
    magic = TL_RPC_INVOKE_REQ;
  }

  q[qn++] = (int)inet_sockaddr_address(&c->local_endpoint);
  q[qn++] = (int)inet_sockaddr_port(&c->local_endpoint);
  q[qn++] = pid;                // pid
  q[qn++] = now - get_uptime(); // start_time
  q[qn++] = worker_id;          // id
  q[qn++] = ready_cnt++;        // ready_cnt
  qn++;
  send_rpc_query(c, magic, -1, q, qn * 4);
}

static void rpc_send_lease_stats(connection *c) {
  int q[100], qn = 0;
  qn += 2;
  q[qn++] = -1;
  *(process_id_t *)(q + qn) = lease_pid;
  static_assert(sizeof(lease_pid) == 12, "");
  qn += 3;
  *(double *)(q + qn) = precise_now - lease_stats_start_time;
  qn += 2;
  *(double *)(q + qn) = lease_stats_time;
  qn += 2;
  q[qn++] = static_cast<int>(lease_stats_cnt);
  qn++;

  send_rpc_query(c, TL_KPHP_LEASE_STATS, -1, q, qn * 4);
}

static int get_current_actor_id() {
  int cur_target = get_current_target();
  if (cur_target == rpc_proxy_target) {
    return rpc_client_actor;
  }
  if (cur_target == rpc_lease_target) {
    return lease_actor_id;
  }
  return -1;
}

static int is_staging;

FLAG_OPTION_PARSER(OPT_ENGINE_CUSTOM, "staging", is_staging, "kphp sends this info to tasks if running as worker");

static void rpc_send_ready(connection *c) {
  static constexpr int QUERY_INT_BUF_LEN = 1000;
  static int q[QUERY_INT_BUF_LEN];
  int qn = 0;
  qn += 2;
  const auto &lease_mode = vk::singleton<LeaseContext>::get().cur_lease_mode;
  const auto &lease_mode_v2 = vk::singleton<LeaseContext>::get().cur_lease_mode_v2;
  bool use_ready_v3 = lease_mode_v2.has_value();
  bool use_ready_v2 = !use_ready_v3 && (is_staging != 0 || lease_mode.has_value());
  int magic = use_ready_v3 ? TL_KPHP_READY_V3 : (use_ready_v2 ? TL_KPHP_READY_V2 : TL_KPHP_READY);

  q[qn++] = -1; // will be replaced by op
  int actor_id = get_current_actor_id();
  if (actor_id != -1) { // we don't want to update all tasks
    qn += wrap_rpc_dest_actor_raw(q + qn, actor_id, magic);
    magic = TL_RPC_INVOKE_REQ;
  }

  if (use_ready_v2) {
    int fields_mask = is_staging ? vk::tl::kphp::ready_v2_fields_mask::is_staging : 0;
    switch (get_lease_mode(lease_mode)) {
      case LeaseWorkerMode::QUEUE_TYPES: {
        fields_mask |= vk::tl::kphp::ready_v2_fields_mask::worker_mode;
        q[qn++] = fields_mask;
        int stored_size = vk::tl::store_to_buffer(reinterpret_cast<char *>(q + qn), (QUERY_INT_BUF_LEN - qn) * 4, lease_mode.value());
        assert(stored_size % 4 == 0);
        qn += stored_size / 4;
        break;
      }
      case LeaseWorkerMode::ALL_QUEUES: {
        q[qn++] = fields_mask;
        break;
      }
    }
  } else if (use_ready_v3) {
    int fields_mask = is_staging ? vk::tl::kphp::ready_v3_fields_mask::is_staging : 0;
    switch (get_lease_mode(lease_mode_v2)) {
      case LeaseWorkerMode::QUEUE_TYPES: {
        fields_mask |= vk::tl::kphp::ready_v3_fields_mask::worker_mode;
        q[qn++] = fields_mask;
        int stored_size = vk::tl::store_to_buffer(reinterpret_cast<char *>(q + qn), (QUERY_INT_BUF_LEN - qn) * 4, lease_mode_v2.value());
        assert(stored_size % 4 == 0);
        qn += stored_size / 4;
        break;
      }
      case LeaseWorkerMode::ALL_QUEUES: {
        q[qn++] = fields_mask;
        break;
      }
    }
  }

  q[qn++] = static_cast<int>(inet_sockaddr_address(&c->local_endpoint));
  q[qn++] = static_cast<int>(inet_sockaddr_port(&c->local_endpoint));
  q[qn++] = pid;                // pid
  q[qn++] = now - get_uptime(); // start_time
  q[qn++] = worker_id;          // id
  q[qn++] = ready_cnt++;        // ready_cnt
  qn++;
  send_rpc_query(c, magic, -1, q, qn * 4);
}

static int rpct_ready(int target_fd) {
  if (target_fd == -1) {
    return -1;
  }
  conn_target_t *target = &Targets[target_fd];
  connection *conn = get_target_connection(target, 0);
  if (conn == nullptr) {
    return -2;
  }
  rpc_send_ready(conn);
  return 0;
}

static void rpct_stop_ready(int target_fd) {
  if (target_fd == -1) {
    return;
  }
  conn_target_t *target = &Targets[target_fd];
  connection *conn = get_target_connection(target, 0);
  if (conn != nullptr) {
    rpc_send_stopped(conn);
  }
}

static void rpct_lease_stats(int target_fd) {
  if (target_fd == -1) {
    return;
  }
  conn_target_t *target = &Targets[target_fd];
  connection *conn = get_target_connection(target, 0);
  if (conn != nullptr) {
    rpc_send_lease_stats(conn);
  }
}

int get_current_target() {
  if (lease_state == lease_state_t::off) {
    // rpc-proxy
    return rpc_proxy_target;
  }
  if (lease_state == lease_state_t::on) {
    // tasks
    return rpc_lease_target;
  }
  return -1;
}

static int lease_off() {
  assert(lease_state == lease_state_t::off);
  if (!lease_ready_flag) {
    return 0;
  }
  if (has_pending_scripts()) {
    return 0;
  }
  set_main_target(RpcClients::get().get_random_alive_client());
  // query the rpc-proxy to get the tasks pid
  if (rpct_ready(rpc_proxy_target) >= 0) {
    tvkprintf(lease, 1, "Start lease cycle: send RPC_READY to balancer proxy\n");
    lease_ready_flag = false;
    return 1;
  }
  return 0;
}

static int lease_start() {
  assert(lease_state == lease_state_t::start);
  if (has_pending_scripts()) {
    return 0;
  }
  lease_change_state(lease_state_t::on);
  lease_ready_flag = true;
  if (rpc_stopped) {
    lease_change_state(lease_state_t::initiating_finish);
  }
  return 1;
}

static int lease_on() {
  assert(lease_state == lease_state_t::on);
  if (!lease_ready_flag) {
    return 0;
  }
  if (has_pending_scripts()) {
    return 0;
  }
  // query the tasks engine to get new tasks
  if (rpct_ready(rpc_lease_target) >= 0) {
    lease_ready_flag = false;
    return 1;
  }
  return 0;
}

static int lease_initiating_finish() {
  assert(lease_state == lease_state_t::initiating_finish);
  if (has_pending_scripts()) {
    return 0;
  }
  rpct_stop_ready(rpc_lease_target);
  stop_ready_ack_timer.start();

  rpct_lease_stats(rpc_proxy_target);
  lease_change_state(lease_state_t::finish);
  lease_ready_flag = false; // waiting TL_KPHP_STOP_READY_ACKNOWLEDGMENT
  return 1;
}

static int lease_finish() {
  assert(lease_state == lease_state_t::finish);
  if (has_pending_scripts()) {
    return 0;
  }
  if (stop_ready_ack_received || is_stop_ready_timeout_expired()) {
    lease_change_state(lease_state_t::off);
    lease_ready_flag = true;

    stop_ready_ack_received = false;
    stop_ready_ack_timer.reset();
    return 1;
  } else {
    lease_ready_flag = false; // waiting TL_KPHP_STOP_READY_ACKNOWLEDGMENT,
                              // then lease_ready_flag set to 1 in rpcx_execute on ACK receiving or run run_rpc_lease directly in lease_cron on timeout expiring
    return 0;
  }
}

void run_rpc_lease() {
  int run_flag = 1;
  while (run_flag) {
    switch (lease_state) {
      case lease_state_t::off:
        run_flag = lease_off();
        break;
      case lease_state_t::start:
        run_flag = lease_start();
        break;
      case lease_state_t::on:
        run_flag = lease_on();
        break;
      case lease_state_t::initiating_finish:
        run_flag = lease_initiating_finish();
        break;
      case lease_state_t::finish:
        run_flag = lease_finish();
        break;
      default:
        assert(0);
    }
  }
}

void lease_cron() {
  int need = 0;

  if (lease_state == lease_state_t::on && rpc_lease_timeout < precise_now) {
    tvkprintf(lease, 1, "Lease time is over, send STOP_READY and initiate finish\n");
    lease_change_state(lease_state_t::initiating_finish);
    need = 1;
  }

  if (lease_state == lease_state_t::finish && is_stop_ready_timeout_expired()) {
    tvkprintf(lease, 1, "STOP_READY_ACKNOWLEDGMENT wasn't received, finish lease hardly\n");
    need = 1;
  }

  if (lease_ready_flag) {
    need = 1;
  }
  if (need) {
    run_rpc_lease();
  }
}

void do_rpc_stop_lease() {
  if (lease_state != lease_state_t::on) {
    return;
  }
  lease_change_state(lease_state_t::initiating_finish);
  run_rpc_lease();
}

int do_rpc_start_lease(process_id_t pid, double timeout, int actor_id) {
  tvkprintf(lease, 1, "Got START_LEASE: pid = %s, timeout = %f, actor_id = %d\n", pid_to_print(&pid), timeout - precise_now, actor_id);
  if (rpc_proxy_target == -1) {
    return -1;
  }

  if (lease_state != lease_state_t::off) {
    return -1;
  }
  int target_fd = get_lease_target_by_pid(pid.ip, pid.port, &rpc_client_ct);
  if (target_fd == -1) {
    return -1;
  }
  if (target_fd == rpc_proxy_target) {
    vkprintf(0, "can't lease to itself\n");
    return -1;
  }
  if (rpc_stopped) {
    return -1;
  }

  rpc_lease_target = target_fd;
  rpc_lease_timeout = timeout;
  lease_pid = pid;
  lease_actor_id = actor_id;

  lease_stats_cnt = 0;
  lease_stats_start_time = precise_now;
  lease_stats_time = 0;

  lease_change_state(lease_state_t::start);
  run_rpc_lease();

  return 0;
}

void do_rpc_finish_lease() {
  if (lease_state != lease_state_t::finish) {
    return;
  }
  stop_ready_ack_received = true;
  tvkprintf(lease, 1, "Got STOP_READY_ACKNOWLEDGMENT, finish lease\n");
  run_rpc_lease();
}

void lease_set_ready() {
  lease_ready_flag = true;
}

void lease_on_stop() {
  if (rpc_proxy_target != -1) {
    conn_target_t *target = &Targets[rpc_proxy_target];
    connection *conn = get_target_connection(target, 0);
    if (conn != nullptr) {
      rpc_send_stopped(conn);
    }
    do_rpc_stop_lease();
  }
}

void set_main_target(const LeaseRpcClient &client) {
  rpc_client_actor = client.actor;
  rpc_proxy_target = client.target_id;
}

connection *get_lease_connection() {
  if (lease_state != lease_state_t::on || rpc_lease_target == -1) {
    return nullptr;
  }
  return get_target_connection(&Targets[rpc_lease_target], 0);
}

process_id_t get_lease_pid() {
  return lease_pid;
}

process_id_t get_rpc_main_target_pid() {
  if (rpc_proxy_target != -1) {
    if (auto *c = get_target_connection(&Targets[rpc_proxy_target], 0)) {
      return TCP_RPC_DATA(c)->remote_pid;
    }
  }
  return {};
}

lease_worker_settings try_fetch_lookup_custom_worker_settings() {
  if (tl_fetch_unread() < 4) {
    return {};
  }
  tl_fetch_mark();
  const int magic = tl_fetch_int();
  if (magic != TL_KPHP_PROCESS_LEASE_TASK && magic != TL_KPHP_PROCESS_LEASE_TASK_V2) {
    tl_fetch_mark_restore();
    return {};
  }
  lease_worker_settings settings;
  settings.fields_mask = tl_fetch_int();
  if (settings.has_timeout()) {
    settings.php_timeout_ms = tl_fetch_int();
  }
  tl_fetch_mark_restore();
  return settings;
}
