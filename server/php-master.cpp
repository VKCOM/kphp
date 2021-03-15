// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "server/php-master.h"

#include <algorithm>
#include <cassert>
#include <cerrno>
#include <climits>
#include <cstdio>
#include <cstdlib>
#include <fcntl.h>
#include <poll.h>
#include <semaphore.h>
#include <string>
#include <sys/mman.h>
#include <sys/signalfd.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/un.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stack>
#include <vector>

#include "common/algorithms/clamp.h"
#include "common/algorithms/find.h"
#include "common/crc32c.h"
#include "common/dl-utils-lite.h"
#include "common/kprintf.h"
#include "common/macos-ports.h"
#include "common/pipe-utils.h"
#include "common/precise-time.h"
#include "common/server/limits.h"
#include "common/server/signals.h"
#include "common/server/stats.h"
#include "common/server/statsd-client.h"
#include "common/timer.h"
#include "common/tl/methods/rwm.h"
#include "common/tl/parse.h"
#include "net/net-connections.h"
#include "net/net-http-server.h"
#include "net/net-memcache-server.h"
#include "net/net-socket.h"
#include "net/net-tcp-rpc-client.h"
#include "net/net-tcp-rpc-server.h"

#include "runtime/confdata-global-manager.h"
#include "runtime/instance-cache.h"
#include "runtime/job-workers/shared-memory-manager.h"
#include "server/confdata-binlog-replay.h"
#include "server/php-engine-vars.h"
#include "server/php-engine.h"
#include "server/php-worker-stats.h"
#include "server/php-master-tl-handlers.h"

#include "server/php-master-restart.h"
#include "server/php-master-warmup.h"

#include "server/job-workers/job-worker-client.h"
#include "server/job-workers/job-workers-context.h"
#include "server/job-workers/job-stats.h"

using job_workers::JobWorkersContext;

extern const char *engine_tag;

//do not kill more then MAX_KILL at the same time
#define MAX_KILL 5

#define PHP_MASTER_VERSION "0.1"

static std::string socket_name, shmem_name;
static int cpu_cnt;

static sigset_t mask;
static sigset_t orig_mask;
static sigset_t empty_mask;

static double my_now;

/*** Shared data functions ***/
void mem_info_add(mem_info_t *dst, const mem_info_t &other) {
  dst->vm_peak += other.vm_peak;
  dst->vm += other.vm;
  dst->rss_peak += other.rss_peak;
  dst->rss += other.rss;
  // do not accumulate rss_file and rss_shmem,
  // because they are about shared memory and accumulated value will show strange stat
}


/*** Stats ***/
static long tot_workers_started{0};
static long tot_workers_dead{0};
static long tot_workers_strange_dead{0};
static long workers_killed{0};
static long workers_hung{0};
static long workers_terminated{0};
static long workers_failed{0};

struct CpuStatTimestamp {
  double timestamp;

  unsigned long long utime;
  unsigned long long stime;
  unsigned long long total_time;
};

struct CpuStat {
  double cpu_usage = 0;
  double cpu_u_usage = 0;
  double cpu_s_usage = 0;

  CpuStat() = default;

  CpuStat(const CpuStatTimestamp &from, const CpuStatTimestamp &to) {
    unsigned long long total_diff = to.total_time - from.total_time;
    if (!total_diff) {
      ++total_diff;
    }
    cpu_u_usage = (double)(to.utime - from.utime) / (double)total_diff;
    cpu_s_usage = (double)(to.stime - from.stime) / (double)total_diff;
    cpu_usage = cpu_u_usage + cpu_s_usage;
  }
};

struct CpuStatSegment {
  using Stat = CpuStat;
  CpuStatTimestamp first, last;

  void init(const CpuStatTimestamp &from) {
    first = from;
    last = from;
  }

  void update(const CpuStatTimestamp &from) {
    last = from;
  }

  double duration() {
    return last.timestamp - first.timestamp;
  }

  Stat get_stat() {
    return {first, last};
  }
};

struct MiscStat {
  int running_workers_max;
  double running_workers_avg;
};

struct MiscStatTimestamp {
  double timestamp;
  int running_workers;
};

struct MiscStatSegment {
  using Stat = MiscStat;

  unsigned long long stat_cnt;
  unsigned long long running_workers_sum;
  int running_workers_max;
  double first_timestamp, last_timestamp;

  void update(const MiscStatTimestamp &from) {
    last_timestamp = from.timestamp;
    stat_cnt++;
    running_workers_sum += from.running_workers;
    if (from.running_workers > running_workers_max) {
      running_workers_max = from.running_workers;
    }
  }

  void init(const MiscStatTimestamp &from) {
    stat_cnt = 0;
    running_workers_max = 0;
    running_workers_sum = 0;
    first_timestamp = from.timestamp;
    update(from);
  }

  double duration() {
    return last_timestamp - first_timestamp;
  }

  MiscStat get_stat() {
    auto running_workers_avg = stat_cnt != 0 ? static_cast<double>(running_workers_sum) / static_cast<double>(stat_cnt) : -1.0;
    return {running_workers_max, running_workers_avg};
  }
};


template<class StatSegment, class StatTimestamp>
struct StatImpl {
  StatSegment first = StatSegment();
  StatSegment second = StatSegment();
  double period = (double)60 * 60 * 24 * 100000;
  bool is_inited = false;

  void set_period(double new_period) {
    period = new_period;
  }

  void add_timestamp(const StatTimestamp &new_timestamp) {
    if (!is_inited) {
      first.init(new_timestamp);
      second.init(new_timestamp);
      is_inited = true;
      return;
    }
    first.update(new_timestamp);
    second.update(new_timestamp);

    if (second.duration() > period) {
      first = second;
      second.init(new_timestamp);
    }
  }

  typename StatSegment::Stat get_stat() {
    if (!is_inited) {
      return typename StatSegment::Stat();
    }
    return first.get_stat();
  }
};

const int periods_n = 4;
const double periods_len[] = {0, 60, 60 * 10, 60 * 60};
const char *periods_desc[] = {"now", "1m", "10m", "1h"};

struct Stats {
  std::string engine_stats;
  std::string cpu_desc, misc_desc;
  php_immediate_stats_t istats = php_immediate_stats_t();
  mem_info_t mem_info = mem_info_t();
  StatImpl<CpuStatSegment, CpuStatTimestamp> cpu[periods_n];
  StatImpl<MiscStatSegment, MiscStatTimestamp> misc[periods_n];
  PhpWorkerStats worker_stats;

  Stats() {
    for (int i = 0; i < periods_n; i++) {
      cpu[i].set_period(periods_len[i]);
      misc[i].set_period(periods_len[i]);
    }

    for (auto period : periods_desc) {
      if (!cpu_desc.empty()) {
        cpu_desc += ",";
      }
      cpu_desc += period;
    }
    for (int i = 1; i < periods_n; i++) {
      if (!misc_desc.empty()) {
        misc_desc += ",";
      }
      misc_desc += periods_desc[i];
    }
  }

  void update(const CpuStatTimestamp &cpu_timestamp) {
    for (auto &i_cpu : cpu) {
      i_cpu.add_timestamp(cpu_timestamp);
    }
  }

  void update(const MiscStatTimestamp &misc_timestamp) {
    for (int i = 1; i < periods_n; i++) {
      misc[i].add_timestamp(misc_timestamp);
    }
  }

  std::string to_string(int pid, bool full_flag = true, bool is_main = false) {
    std::string res;

    if (full_flag) {
      res += engine_stats;
    }

    char buffer[1000];

    std::string pid_s;
    if (!is_main) {
      assert (snprintf(buffer, 1000, " %d", pid) < 1000);
      pid_s = buffer;
    }

//    sprintf (buffer, "is_running = %d\n", istats.is_running);
//    sprintf (buffer, "is_paused = %d\n", istats.is_wait_net);

    std::string cpu_vals;
    for (auto &i_cpu : cpu) {
      CpuStat s = i_cpu.get_stat();

      sprintf(buffer, " %6.2lf%%", cpu_cnt * (s.cpu_usage * 100));
      cpu_vals += buffer;
    }
    res += "cpu_usage" + pid_s + "(" + cpu_desc + ")\t" + cpu_vals + "\n";
    sprintf(buffer, "VM%s\t%lluKb\n", pid_s.c_str(), mem_info.vm);
    res += buffer;
    sprintf(buffer, "VM_max%s\t%lluKb\n", pid_s.c_str(), mem_info.vm_peak);
    res += buffer;
    sprintf(buffer, "RSS%s\t%lluKb\n", pid_s.c_str(), mem_info.rss);
    res += buffer;
    sprintf(buffer, "RSS_max%s\t%lluKb\n", pid_s.c_str(), mem_info.rss_peak);
    res += buffer;

    if (is_main) {
      std::string running_workers_max_vals;
      std::string running_workers_avg_vals;
      for (int i = 1; i < periods_n; i++) {
        MiscStat s = misc[i].get_stat();
        sprintf(buffer, " %7.3lf", s.running_workers_avg);
        running_workers_avg_vals += buffer;
        sprintf(buffer, " %7d", s.running_workers_max);
        running_workers_max_vals += buffer;
      }
      res += "running_workers_avg" + pid_s + "(" + misc_desc + ")\t" + running_workers_avg_vals + "\n";
      res += "running_workers_max" + pid_s + "(" + misc_desc + ")\t" + running_workers_max_vals + "\n";
    }

    if (full_flag) {
      res += worker_stats.to_string(pid_s);
    }

    return res;
  }
};

void run_master();

static volatile long long local_pending_signals = 0;

static void sigusr1_handler(const int sig) {
  const char message[] = "got SIGUSR1, rotate logs.\n";
  kwrite(2, message, sizeof(message) - (size_t)1);

  local_pending_signals = local_pending_signals | (1ll << sig);
}

/**
 * Description of states:
 * on                       - main working state: reruns terminated workers, communicates with old master during graceful restart
 *                                                (manages killing old master's workers, ask resources, etc.).
 * off_in_graceful_restart  - old master's state during graceful restart: gradually kills its workers, hands over resources (http_fd, etc.) to new master
 *                                                                                      and terminates.
 * off_in_graceful_shutdown - graceful shutdown state (FINAL state): gradually kills workers and terminates.
 *
 * See master_change_state about transitions between states.
 */
enum class master_state {
  on,
  off_in_graceful_restart,
  off_in_graceful_shutdown
};

#define MAX_WORKER_STATS_LEN 10000
//TODO: save it as pointer
struct pipe_info_t {
  int pipe_read;
  int pipe_in_packet_num;
  int pipe_out_packet_num;
  connection *reader;
  connection pending_stat_queue;
};

enum class WorkerType {
  http_worker,
  job_worker
};

struct worker_info_t {
  worker_info_t *next_worker;

  int generation;
  pid_t pid;
  int is_dying;

  double last_activity_time;
  double start_time;
  double kill_time;
  int kill_flag;

  int stats_flag;

  pipe_info_t pipes[2];

  pid_info_t my_info;
  int valid_my_info;

  int slot_id;

  Stats *stats;
  WorkerType type;
};


using worker_slots_t = std::stack<int, std::vector<int>>;
static worker_slots_t http_worker_slot_ids;
static worker_slots_t job_worker_slot_ids;

static worker_info_t *workers[MAX_WORKERS];
static Stats server_stats;
static unsigned long long dead_stime, dead_utime;
static PhpWorkerStats dead_worker_stats;
static double last_failed;
static int *http_fd;
static int http_fd_port;
static int (*try_get_http_fd)();
static master_state state;
static bool in_sigterm;

static int signal_fd;

static int changed = 0;
static int failed = 0;
static int socket_fd = -1;
static int to_kill = 0, to_run = 0, to_exit = 0;
static long long generation;
static int receive_fd_attempts_cnt = 0;

static worker_info_t *free_workers = nullptr;

void worker_init(worker_info_t *w) {
  w->stats = new Stats();
  w->valid_my_info = 0;
}

void worker_free(worker_info_t *w) {
  delete w->stats;
  w->stats = nullptr;
}

worker_info_t *new_worker() {
  worker_info_t *w = free_workers;
  if (w == nullptr) {
    w = (worker_info_t *)calloc(1, sizeof(worker_info_t));
  } else {
    free_workers = free_workers->next_worker;
  }
  worker_init(w);
  return w;
}

void delete_worker(worker_info_t *w) {
  w->generation = ++conn_generation;
  switch (w->type) {
    case WorkerType::http_worker:
      http_worker_slot_ids.push(w->slot_id);
      break;
    case WorkerType::job_worker:
      job_worker_slot_ids.push(w->slot_id);
      break;
  }
  if (w->valid_my_info) {
    dead_utime += w->my_info.utime;
    dead_stime += w->my_info.stime;
  }
  dead_worker_stats.add_from(w->stats->worker_stats);
  // ignore dead workers memory and percentiles stats
  dead_worker_stats.reset_memory_and_percentiles_stats();
  worker_free(w);
  w->next_worker = free_workers;
  free_workers = w;
}

void start_master(int *new_http_fd, int (*new_try_get_http_fd)(), int new_http_fd_port) {
  initial_verbosity = verbosity;
  if (verbosity < 1) {
    //verbosity = 1;
  }
  for (int slot_id = MAX_WORKERS - 1; slot_id >= 0; slot_id--) {
    if (slot_id < workers_n) {
      http_worker_slot_ids.push(slot_id);
    } else {
      job_worker_slot_ids.push(slot_id);
    }
  }

  std::string s = cluster_name;
  std::replace_if(s.begin(), s.end(), [](unsigned char c) { return !isalpha(c); }, '_');

  shmem_name = std::string() + "/" + s + "_kphp_shm";
  socket_name = std::string() + s + "_kphp_fd_transfer";

  dl_assert (shmem_name.size() < NAME_MAX, "too long name for shared memory file");
  dl_assert (socket_name.size() < NAME_MAX, "too long socket name (for fd transfer)");
  kprintf("[%s] [%s]\n", shmem_name.c_str(), socket_name.c_str());

  http_fd = new_http_fd;
  http_fd_port = new_http_fd_port;
  try_get_http_fd = new_try_get_http_fd;

  vkprintf(1, "start master: begin\n");

  sigemptyset(&empty_mask);

  //currently all signals are blocked

  sigemptyset(&mask);
  sigaddset(&mask, SIGCHLD);
  sigaddset(&mask, SIGPOLL);
  sigaddset(&mask, SIGTERM);

  signal_fd = signalfd(-1, &mask, SFD_NONBLOCK);
  dl_passert (signal_fd >= 0, "failed to create signalfd");

  ksignal(SIGUSR1, sigusr1_handler);

  //allow all signals except SIGPOLL, SIGCHLD and SIGTERM
  if (sigprocmask(SIG_SETMASK, &mask, &orig_mask) < 0) {
    perror("sigprocmask");
    _exit(1);
  }


  //TODO: other signals, daemonize, change user...
  if (shared_data == nullptr) {
    shared_data = get_shared_data(shmem_name.c_str());
  }

  int attempts_to_start = 2;
  int is_inited = 0;
  while (attempts_to_start-- > 0) {
    vkprintf(1, "attempt to init master. [left attempts = %d]\n", attempts_to_start);
    shared_data_lock(shared_data);

    shared_data_update(shared_data);
    shared_data_get_masters(shared_data, &me, &other);

    if (me != nullptr) {
      master_init(me, other);
      is_inited = 1;
    }

    shared_data_unlock(shared_data);

    if (!is_inited) {
      vkprintf(1, "other restart is in progress. sleep 5 seconds. [left attempts = %d]\n", attempts_to_start);
      sleep(5);
    } else {
      break;
    }
  }

  if (!is_inited) {
    vkprintf(0, "Failed to init master. It seems that two other masters are running\n");
    _exit(1);
  }

  vkprintf(1, "start master: end\n");

  run_master();
}

void terminate_worker(worker_info_t *w) {
  vkprintf(1, "kill_worker: send SIGTERM to [pid = %d]\n", (int)w->pid);
  kill(w->pid, SIGTERM);
  w->is_dying = 1;
  w->kill_time = my_now + 35;
  w->kill_flag = 0;

  switch (w->type) {
    case WorkerType::http_worker:
      workers_terminated++;
      me_running_http_workers_n--;
      me_dying_http_workers_n++;
      changed = 1;
      break;
    case WorkerType::job_worker:
      auto &ctx = vk::singleton<JobWorkersContext>::get();
      ctx.running_job_workers--;
      ctx.dying_job_workers++;
      break;
  }
}

int kill_http_worker() {
  for (int i = 0; i < me_all_workers_n; i++) {
    if (workers[i]->type == WorkerType::http_worker && !workers[i]->is_dying) {
      terminate_worker(workers[i]);
      return 1;
    }
  }

  return 0;
}

static int get_max_hanging_time_sec(WorkerType worker_type) {
  switch (worker_type) {
    case WorkerType::http_worker:
      return max(script_timeout + 1, 65); // + 1 sec for terminating
    case WorkerType::job_worker:
      return JobWorkersContext::MAX_HANGING_TIME_SEC;
  }
  return 0;
}

void kill_hanging_workers() {
  static double last_terminated = -1;
  if (last_terminated + 30 < my_now) {
    for (int i = 0; i < me_all_workers_n; i++) {
      if (!workers[i]->is_dying && workers[i]->last_activity_time + get_max_hanging_time_sec(workers[i]->type) <= my_now) {
        vkprintf(1, "No stats received from worker [pid = %d]. Terminate it\n", (int)workers[i]->pid);
        if (workers[i]->type == WorkerType::job_worker) {
          tvkprintf(job_workers, 1, "No stats received from job worker [pid = %d]. Terminate it\n", (int)workers[i]->pid);
        }
        workers_hung++;
        terminate_worker(workers[i]);
        last_terminated = my_now;
        break;
      }
    }
  }

  for (int i = 0; i < me_all_workers_n; i++) {
    if (workers[i]->is_dying && workers[i]->kill_time <= my_now && workers[i]->kill_flag == 0) {
      vkprintf(1, "kill_hanging_worker: send SIGKILL to [pid = %d]\n", (int)workers[i]->pid);
      if (workers[i]->type == WorkerType::job_worker) {
        tvkprintf(job_workers, 1, "kill hanging job worker: send SIGKILL to [pid = %d]\n", (int)workers[i]->pid);
      }
      kill(workers[i]->pid, SIGKILL);
      workers_killed++;

      workers[i]->kill_flag = 1;

      changed = 1;
    }
  }
}

void workers_send_signal(int sig) {
  for (int i = 0; i < me_all_workers_n; i++) {
    if (!workers[i]->is_dying) {
      kill(workers[i]->pid, sig);
    }
  }
}

void pipe_on_get_packet(pipe_info_t *p, int packet_num) {
  assert (packet_num > p->pipe_in_packet_num);
  p->pipe_in_packet_num = packet_num;
  connection *c = &p->pending_stat_queue;
  while (c->first_query != (conn_query *)c) {
    conn_query *q = c->first_query;
    dl_assert (q != nullptr, "...");
    dl_assert (q->requester != nullptr, "...");
    //    fprintf (stderr, "processing delayed query %p for target %p initiated by %p (%d:%d<=%d)\n", q, c->target, q->requester, q->requester->fd, q->req_generation, q->requester->generation);
    if (q->requester->generation == q->req_generation) {
      int need_packet_num = *(int *)&q->extra;
      //vkprintf (1, "%d vs %d\n", need_packet_num, packet_num);

      //use sign of difference to handle int overflow
      int diff = packet_num - need_packet_num;
      assert (abs(diff) < 1000000000);
      if (diff >= 0) {
        assert (diff == 0);
        //TODO: use PMM_DATA (q->requester);
        q->cq_type->close(q);
      } else {
        break;
      }
    } else {
      q->cq_type->close(q);
    }
  }
}

void worker_set_stats(worker_info_t *w, const char *data) {
  data += w->stats->worker_stats.read_from(data);
  w->stats->engine_stats = data;
}

void worker_set_immediate_stats(worker_info_t *w, php_immediate_stats_t *istats) {
  w->stats->istats = *istats;
}


/*** PIPE connection ***/
struct pr_data {
  worker_info_t *worker;
  int worker_generation;
  pipe_info_t *pipe_info;
};
#define PR_DATA(c) ((pr_data *)(TCP_RPC_DATA (c) + 1))

int pr_execute(connection *c, int op, raw_message *raw) {
  vkprintf(3, "pr_execute: fd=%d, op=%d, len=%d\n", c->fd, op, raw->total_bytes);

  if (vk::none_of_equal(op, RPC_PHP_FULL_STATS, RPC_PHP_IMMEDIATE_STATS)) {
    return 0;
  }
  auto len = raw->total_bytes;

  assert(len % sizeof(int) == 0);
  assert(len >= sizeof(int));

  tl_fetch_init_raw_message(raw);
  auto op_from_tl = tl_fetch_int();
  len -= static_cast<int>(sizeof(op_from_tl));
  assert(op_from_tl == op);

  std::vector<char> buf(len);
  auto fetched_bytes = tl_fetch_data(buf.data(), static_cast<int>(buf.size()));
  assert(fetched_bytes == buf.size());

  worker_info_t *w = PR_DATA(c)->worker;
  pipe_info_t *p = PR_DATA(c)->pipe_info;
  if (w->generation == PR_DATA(c)->worker_generation) {
    if (op == RPC_PHP_FULL_STATS) {
      w->last_activity_time = my_now;
      worker_set_stats(w, buf.data());
    }

    if (op == RPC_PHP_IMMEDIATE_STATS) {
      worker_set_immediate_stats(w, reinterpret_cast<php_immediate_stats_t *>(buf.data()));
    }

    pipe_on_get_packet(p, TCP_RPC_DATA(c)->packet_num);
  } else {
    vkprintf(1, "connection [%p:%d] will be closed soon\n", c, c->fd);
  }

  return 0;
}

tcp_rpc_client_functions pipe_reader_methods = [] {
  auto res = tcp_rpc_client_functions();
  res.execute = pr_execute;

  return res;
}();

void init_pipe_info(pipe_info_t *info, worker_info_t *worker, int pipe) {
  info->pipe_read = pipe;
  info->pipe_out_packet_num = -1;
  info->pipe_in_packet_num = -1;
  connection *reader = epoll_insert_pipe(pipe_for_read, pipe, &ct_tcp_rpc_client, (void *)&pipe_reader_methods);
  if (reader != nullptr) {
    PR_DATA (reader)->worker = worker;
    PR_DATA (reader)->worker_generation = worker->generation;
    PR_DATA (reader)->pipe_info = info;
  } else {
    vkprintf(0, "failed to create pipe_reader [fd = %d]", pipe);
  }
  info->reader = reader;

  connection *c = &info->pending_stat_queue;
  c->first_query = c->last_query = (conn_query *)c;
  c->generation = ++conn_generation;
  c->pending_queries = 0;
}

void clear_pipe_info(pipe_info_t *info) {
  info->pipe_read = -1;
  info->reader = nullptr;
}

int run_worker(WorkerType worker_type) {
  dl_block_all_signals();

  int err;
  assert (me_all_workers_n < MAX_WORKERS);

  int new_pipe[2];
  err = pipe2(new_pipe, O_CLOEXEC);
  dl_assert (err != -1, "failed to create a pipe");
  int new_fast_pipe[2];
  err = pipe2(new_fast_pipe, O_CLOEXEC);
  dl_assert (err != -1, "failed to create a pipe");

  tot_workers_started++;

  pid_t new_pid = fork();
  assert (new_pid != -1 && "failed to fork");

  int worker_slot_id = -1;
  switch (worker_type) {
    case WorkerType::http_worker:
      worker_slot_id = http_worker_slot_ids.top();
      http_worker_slot_ids.pop();
      break;
    case WorkerType::job_worker:
      worker_slot_id = job_worker_slot_ids.top();
      job_worker_slot_ids.pop();
      break;
  }
  if (new_pid == 0) {
    switch (worker_type) {
      case WorkerType::http_worker:
        run_mode = RunMode::http_worker;
        break;
      case WorkerType::job_worker:
        run_mode = RunMode::job_worker;
        break;
    }
    prctl(PR_SET_PDEATHSIG, SIGKILL); // TODO: or SIGTERM
    if (getppid() != me->pid) {
      vkprintf(0, "parent is dead just after start\n");
      exit(123);
    }

    // TODO should we just use net_reset_after_fork()?

    //Epoll_close should clear internal structures but shouldn't change epoll_fd.
    //The same epoll_fd will be used by master
    //Solution: close epoll_fd first
    //Problems: "epoll_ctl(): Invalid argument" is printed to stderr
    close_epoll();
    init_epoll();

    //verbosity = 0;
    verbosity = initial_verbosity;
    pid = getpid();

    master_pipe_write = new_pipe[1];
    master_pipe_fast_write = new_fast_pipe[1];
    close(new_pipe[0]);
    close(new_fast_pipe[0]);
    clear_event(new_pipe[0]);
    clear_event(new_fast_pipe[0]);

    master_sfd = -1;

    for (int i = 0; i < allocated_targets; i++) {
      while (Targets[i].refcnt > 0) {
        destroy_target(&Targets[i]);
      }
    }

    for (int i = 0; i < MAX_CONNECTIONS; i++) {
      connection_t *conn = &Connections[i];
      if (conn->status == conn_none) {
        continue;
      }
      close(i);
      clear_event(i);
      force_clear_connection(conn);
    }

    active_outbound_connections = 0;
    active_connections = 0;

    reset_PID();

    // TODO: fill other stats with zero
    //

    signal_fd = -1;
    logname_id = worker_slot_id;
    if (logname_pattern) {
      char buf[100];
      snprintf(buf, 100, logname_pattern, worker_slot_id);
      logname = strdup(buf);
    }

    instance_cache_release_all_resources_acquired_by_this_proc();
    ConfdataGlobalManager::get().force_release_all_resources_acquired_by_this_proc_if_init();
    return 1;
  }

  dl_restore_signal_mask();

  vkprintf(1, "new worker launched [pid = %d]\n", (int)new_pid);

  worker_info_t *worker = workers[me_all_workers_n++] = new_worker();
  worker->pid = new_pid;

  worker->is_dying = 0;
  worker->generation = ++conn_generation;
  worker->start_time = my_now;
  worker->slot_id = worker_slot_id;
  worker->last_activity_time = my_now;
  worker->type = worker_type;

  init_pipe_info(&worker->pipes[0], worker, new_pipe[0]);
  init_pipe_info(&worker->pipes[1], worker, new_fast_pipe[0]);

  close(new_pipe[1]);
  close(new_fast_pipe[1]);

  switch (worker_type) {
    case WorkerType::http_worker:
      me_running_http_workers_n++;
      break;
    case WorkerType::job_worker:
      vk::singleton<JobWorkersContext>::get().running_job_workers++;
      break;
  }

  changed = 1;

  return 0;
}

void remove_worker(pid_t pid) {
  vkprintf(2, "remove workers [pid = %d]\n", (int)pid);
  for (int i = 0; i < me_all_workers_n; i++) {
    if (workers[i]->pid == pid) {
      switch (workers[i]->type) {
        case WorkerType::http_worker:
          if (workers[i]->is_dying) {
            me_dying_http_workers_n--;
          } else {
            me_running_http_workers_n--;
            last_failed = my_now;
            failed++;
            workers_failed++;
          }
          break;
        case WorkerType::job_worker:
          auto &job_workers_ctx = vk::singleton<JobWorkersContext>::get();
          if (workers[i]->is_dying) {
            job_workers_ctx.dying_job_workers--;
          } else {
            job_workers_ctx.running_job_workers--;
          }
          break;
      }

      clear_pipe_info(&workers[i]->pipes[0]);
      clear_pipe_info(&workers[i]->pipes[1]);
      delete_worker(workers[i]);

      me_all_workers_n--;
      workers[i] = workers[me_all_workers_n];

      vkprintf(1, "worker_removed: [running = %d] [dying = %d]\n", me_running_http_workers_n, me_dying_http_workers_n);
      return;
    }
  }

  assert (0 && "trying to remove unexisted worker");
}

void update_workers() {
  while (true) {
    int status;
    pid_t pid = waitpid(-1, &status, WNOHANG);
    if (pid > 0) {
      if (!WIFEXITED (status)) {
        tot_workers_strange_dead++;
      }
      tot_workers_dead++;
      remove_worker(pid);
      changed = 1;
    } else {
      break;
    }
  }
}


/*** send fd via unix socket ***/
void init_sockaddr_un(sockaddr_un *unix_socket_addr, const char *name) {
  memset(unix_socket_addr, 0, sizeof(*unix_socket_addr));
  unix_socket_addr->sun_family = AF_LOCAL;
  dl_assert (strlen(name) < sizeof(unix_socket_addr->sun_path), "too long socket name");
  strcpy(unix_socket_addr->sun_path, name);
}

static const sockaddr_un *get_socket_addr() {

  static sockaddr_un unix_socket_addr;
  static int inited = 0;

  if (!inited) {
    init_sockaddr_un(&unix_socket_addr, socket_name.c_str());
    inited = 1;
  }

  return &unix_socket_addr;
}

static int send_fd_via_socket(int fd) {
  int unix_socket_fd = socket(AF_LOCAL, SOCK_DGRAM, 0);
  dl_passert (fd >= 0, "failed to create socket");

  msghdr msg;
  char ccmsg[CMSG_SPACE(sizeof(fd))];
  cmsghdr *cmsg;
  iovec vec;  /* stupidity: must send/receive at least one byte */
  const char *str = "x";
  int rv;

  msg.msg_name = (sockaddr *)get_socket_addr();
  msg.msg_namelen = sizeof(*get_socket_addr());

  vec.iov_base = (void *)str;
  vec.iov_len = 1;
  msg.msg_iov = &vec;
  msg.msg_iovlen = 1;

  /* old BSD implementations should use msg_accrights instead of
   * msg_control; the interface is different. */
  msg.msg_control = ccmsg;
  msg.msg_controllen = sizeof(ccmsg);
  cmsg = CMSG_FIRSTHDR (&msg);
  cmsg->cmsg_level = SOL_SOCKET;
  cmsg->cmsg_type = SCM_RIGHTS;
  cmsg->cmsg_len = CMSG_LEN (sizeof(fd));
  *(int *)CMSG_DATA (cmsg) = fd;
  msg.msg_controllen = cmsg->cmsg_len;

  msg.msg_flags = 0;

  rv = (sendmsg(unix_socket_fd, &msg, 0) != -1);
  if (rv) {
    //why?
    //close (fd);
  } else {
    perror("failed to send http_fd (sendmsg)");
  }
  return rv;
}

static int sock_dgram(const char *path) {
  int err = unlink(path);
  dl_passert (err >= 0 || errno == ENOENT, dl_pstr("failed to unlink %s", path));

  int fd = socket(PF_UNIX, SOCK_DGRAM, 0);
  fcntl(fd, F_SETFL, O_NONBLOCK);
  dl_passert (fd != -1, "failed to create a socket");
  err = bind(fd, (sockaddr *)get_socket_addr(), sizeof(*get_socket_addr()));
  dl_passert (err >= 0, "failed to bind socket");
  return fd;
}

/* receive a file descriptor over file descriptor fd */
static int receive_fd(int fd) {
  msghdr msg;
  iovec iov;
  char buf[1];
  int rv;
  int connfd = -1;
  char ccmsg[CMSG_SPACE (sizeof(connfd))];
  cmsghdr *cmsg;

  iov.iov_base = buf;
  iov.iov_len = 1;

  msg.msg_name = 0;
  msg.msg_namelen = 0;
  msg.msg_iov = &iov;
  msg.msg_iovlen = 1;
  /* old BSD implementations should use msg_accrights instead of
   * msg_control; the interface is different. */
  msg.msg_control = ccmsg;
  msg.msg_controllen = sizeof(ccmsg); /* ? seems to work... */

  rv = (int)recvmsg(fd, &msg, 0);
  if (rv == -1) {
    perror("recvmsg");
    return -1;
  }

  cmsg = CMSG_FIRSTHDR (&msg);
  if (cmsg->cmsg_type != SCM_RIGHTS) {
    fprintf(stderr, "got control message of unknown type %d\n",
            cmsg->cmsg_type);
    return -1;
  }
  return *(int *)CMSG_DATA (cmsg);
}


/*** Memcached interface for stats ***/
int php_master_get(connection *c, const char *key, int key_len);
int php_master_version(connection *c);
int php_master_wakeup(connection *c);
int php_master_get_end(connection *c, int key_cnt);

memcache_server_functions php_master_methods = [] {
  auto res = memcache_server_functions();
  res.execute = mcs_execute;
  res.mc_store = mcs_store;
  res.mc_get_start = mcs_get_start;
  res.mc_get = php_master_get;
  res.mc_get_end = php_master_get_end;
  res.mc_incr = mcs_incr;
  res.mc_delete = mcs_delete;
  res.mc_version = php_master_version;
  res.mc_stats = mcs_stats;
  res.mc_check_perm = mcs_default_check_perm;
  res.mc_init_crypto = mcs_init_crypto;
  res.mc_wakeup = php_master_wakeup;
  res.mc_alarm = php_master_wakeup;

  return res;
}();

/*** HTTP interface for server-status ***/
int php_master_http_execute(struct connection *c, int op);

http_server_functions php_http_master_methods = [] {
  auto res = http_server_functions();
  res.execute = php_master_http_execute;
  res.ht_wakeup = hts_do_wakeup; //todo: assert false here!!!
  res.ht_alarm = hts_do_wakeup; //todo: assert false here!!!
  res.ht_close = nullptr;

  return res;
}();

tcp_rpc_server_functions php_rpc_master_methods = [] {
  auto res = tcp_rpc_server_functions();
  res.execute = master_rpc_tl_execute;
  res.check_ready = server_check_ready;
  res.flush_packet = tcp_rpcs_flush_packet;
  res.rpc_check_perm = tcp_rpcs_default_check_perm;
  res.rpc_init_crypto = tcp_rpcs_init_crypto;
  res.memcache_fallback_type = &ct_memcache_server;
  res.memcache_fallback_extra = &php_master_methods;
  res.http_fallback_type = &ct_http_server;
  res.http_fallback_extra = &php_http_master_methods;
  return res;
}();

struct pmm_data {
  int full_flag;
  int worker_pid;
  int need_end;
};

#define PMM_DATA(c) ((pmm_data *) (MCS_DATA(c) + 1))
int delete_stats_query(conn_query *q);

conn_query_functions stats_cq_func = [] {
  auto res = conn_query_functions();
  res.magic = CQUERY_FUNC_MAGIC;
  res.title = "stats-cq-query";
  res.wakeup = delete_stats_query;
  res.close = delete_stats_query;

  return res;
}();

int delete_stats_query(conn_query *q) {
  vkprintf(2, "delete_stats_query(%p,%p)\n", q, q->requester);

  delete_conn_query(q);
  free(q);
  return 0;
}

conn_query *create_stats_query(connection *c, pipe_info_t *pipe_info) {
  auto q = reinterpret_cast<conn_query *>(malloc(sizeof(conn_query)));

  q->custom_type = 0;
  q->outbound = &pipe_info->pending_stat_queue;
  q->requester = c;
  q->start_time = precise_now;

  *(int *)&q->extra = pipe_info->pipe_out_packet_num;

  q->cq_type = &stats_cq_func;
  q->timer.wakeup_time = precise_now + 1;
  vkprintf(2, "create stats query [q = %p] [requester = %p]\n", q, q->requester);

  insert_conn_query(q);

  return q;
}

void create_stats_queries(connection *c, int op, int worker_pid) {
#if defined(__APPLE__)
  op |= SPOLL_SEND_IMMEDIATE_STATS | SPOLL_SEND_FULL_STATS;
#endif
  sigval to_send;
  to_send.sival_int = op;
  for (int i = 0; i < me_all_workers_n; i++) {
    workers[i]->stats_flag = 0;
    if (!workers[i]->is_dying && (worker_pid < 0 || workers[i]->pid == worker_pid)) {
      pipe_info_t *pipe_info = nullptr;
      if (op & SPOLL_SEND_IMMEDIATE_STATS) {
        pipe_info = &workers[i]->pipes[1];
      } else if (op & SPOLL_SEND_FULL_STATS) {
        pipe_info = &workers[i]->pipes[0];
      }
      dl_assert (pipe_info != nullptr, "bug in code");
#if defined(__APPLE__)
      kill(workers[i]->pid, SIGSTAT);
#else
      sigqueue(workers[i]->pid, SIGSTAT, to_send);
#endif
      pipe_info->pipe_out_packet_num++;
      vkprintf(1, "create_stats_query [worker_pid = %d], [packet_num = %d]\n", workers[i]->pid, pipe_info->pipe_out_packet_num);
      if (c != nullptr) {
        create_stats_query(c, pipe_info);
      }
    }
  }
  if (c != nullptr) {
    c->status = conn_wait_net;
  }
}

int return_one_key_key(connection *c, const char *key) {
  std::string tmp;
  tmp += "VALUE ";
  tmp += key;
  write_out(&c->Out, tmp.c_str(), (int)tmp.size());
  return 0;
}

int return_one_key_val(connection *c, const char *val, int vlen) {
  char tmp[300];
  int l = sprintf(tmp, " 0 %d\r\n", vlen);
  assert (l < 300);
  write_out(&c->Out, tmp, l);
  write_out(&c->Out, val, vlen);
  write_out(&c->Out, "\r\n", 2);
  return 0;
}

int update_mem_stats();

std::string php_master_prepare_stats(bool full_flag, int worker_pid) {
  std::string res, header;
  header = server_stats.to_string(me == nullptr ? 0 : (int)me->pid, false, true);
  int total_workers_n = 0;
  int running_workers_n = 0;
  int paused_workers_n = 0;
  static char buf[1000];

  PhpWorkerStats worker_stats;

  double min_uptime = 1e9;
  double max_uptime = -1;
  for (int i = 0; i < me_all_workers_n; i++) {
    worker_info_t *w = workers[i];
    if (!w->is_dying) {
      total_workers_n++;
      running_workers_n += w->stats->istats.is_running;
      paused_workers_n += w->stats->istats.is_wait_net;
      double worker_uptime = my_now - w->start_time;
      if (min_uptime > worker_uptime) {
        min_uptime = worker_uptime;
      }
      if (max_uptime < worker_uptime) {
        max_uptime = worker_uptime;
      }

      if (full_flag) {
        worker_stats.add_from(w->stats->worker_stats);
      }

      if (worker_pid == -1 || w->pid == worker_pid) {
        sprintf(buf, "worker_uptime %d\t%.0lf\n", (int)w->pid, worker_uptime);
        res += buf;
        res += w->stats->to_string(w->pid, full_flag);
        res += "\n";
      }
    }
  }

  sprintf(buf, "uptime\t%d\n", get_uptime());
  header += buf;
  if (engine_tag != nullptr) {
    sprintf(buf + sprintf(buf, "kphp_version\t%s", engine_tag) - 2, "\n");
    header += buf;
  }
  sprintf(buf, "cluster_name\t%s\n", cluster_name);
  header += buf;
  sprintf(buf, "min_worker_uptime\t%.0lf\n", min_uptime);
  header += buf;
  sprintf(buf, "max_worker_uptime\t%.0lf\n", max_uptime);
  header += buf;
  sprintf(buf, "total_workers\t%d\n", total_workers_n);
  header += buf;
  sprintf(buf, "running_workers\t%d\n", running_workers_n);
  header += buf;
  sprintf(buf, "paused_workers\t%d\n", paused_workers_n);
  header += buf;
  sprintf(buf, "dying_workers\t%d\n", me_dying_http_workers_n);
  header += buf;
  sprintf(buf, "tot_workers_started\t%ld\n", tot_workers_started);
  header += buf;
  sprintf(buf, "tot_workers_dead\t%ld\n", tot_workers_dead);
  header += buf;
  sprintf(buf, "tot_workers_strange_dead\t%ld\n", tot_workers_strange_dead);
  header += buf;
  sprintf(buf, "workers_killed\t%ld\n", workers_killed);
  header += buf;
  sprintf(buf, "workers_hung\t%ld\n", workers_hung);
  header += buf;
  sprintf(buf, "workers_terminated\t%ld\n", workers_terminated);
  header += buf;
  sprintf(buf, "workers_failed\t%ld\n", workers_failed);
  header += buf;

  if (full_flag) {
    header += worker_stats.to_string();
  }
  if (!full_flag && worker_pid == -2) {
    header += " pid \t  state time\t  port  actor time\tcustom_server_status time\n";
    for (int i = 0; i < me_all_workers_n; i++) {
      worker_info_t *w = workers[i];
      if (!w->is_dying) {
        php_immediate_stats_t *imm = &w->stats->istats;
        imm->desc[IMM_STATS_DESC_LEN - 1] = 0;
        imm->custom_desc[IMM_STATS_DESC_LEN - 1] = 0;
        sprintf(buf, "%5d\t%7s %.3lf\t%6d %6lld %.3lf\t%s %.3lf\n",
                w->pid, imm->desc, precise_now - imm->timestamp,
                imm->port, imm->actor_id, precise_now - imm->rpc_timestamp,
                imm->custom_desc, precise_now - imm->custom_timestamp);
        header += buf;
      }
    }
  }
  return header + res;
}


int php_master_wakeup(connection *c) {
  if (c->status == conn_wait_net) {
    c->status = conn_expect_query;
  }

  pmm_data *D = PMM_DATA (c);
  bool full_flag = D->full_flag;
  int worker_pid = D->worker_pid;

  update_workers();
  update_mem_stats();

  std::string res = php_master_prepare_stats(full_flag, worker_pid);
  return_one_key_val(c, (char *)res.c_str(), (int)res.size());
  if (D->need_end) {
    write_out(&c->Out, "END\r\n", 5);
  }

  mcs_pad_response(c);
  c->flags |= C_WANTWR;

  return 0;
}

inline void eat_at(const char *key, int key_len, char **new_key, int *new_len) {
  if (*key == '^' || *key == '!') {
    key++;
    key_len--;
  }

  *new_key = (char *)key;
  *new_len = key_len;

  if ((*key >= '0' && *key <= '9') || (*key == '-' && key[1] >= '0' && key[1] <= '9')) {
    key++;
    while (*key >= '0' && *key <= '9') {
      key++;
    }

    if (*key++ == '@') {
      if (*key == '^' || *key == '!') {
        key++;
      }

      *new_len -= (int)(key - *new_key);
      *new_key = (char *)key;
    }
  }
}

int php_master_get(connection *c, const char *old_key, int old_key_len) {
  char *key;
  int key_len;
  eat_at(old_key, old_key_len, &key, &key_len);
  //stats
  //full_stats
  //workers_pids
  if (key_len == 12 && strncmp(key, "workers_pids", 12) == 0) {
    std::string res;
    for (int i = 0; i < workers_n; i++) {
      if (!workers[i]->is_dying) {
        char buf[30];
        sprintf(buf, "%d", workers[i]->pid);
        if (!res.empty()) {
          res += ",";
        }
        res += buf;
      }
    }
    return_one_key(c, old_key, (char *)res.c_str(), (int)res.size());
    return 0;
  }
  if (key_len >= 5 && strncmp(key, "stats", 5) == 0) {
    key += 5;
    pmm_data *D = PMM_DATA (c);
    D->full_flag = 0;
    if (strncmp(key, "_full", 5) == 0) {
      key += 5;
      D->full_flag = 1;
      D->worker_pid = -1;
      sscanf(key, "%d", &D->worker_pid);
    } else if (strncmp(key, "_fast", 5) == 0) {
      D->worker_pid = -2;
      key += 5;
      sscanf(key, "%d", &D->worker_pid);
    } else {
      D->full_flag = 1;
      D->worker_pid = -2;
    }

    create_stats_queries(c, SPOLL_SEND_STATS | SPOLL_SEND_IMMEDIATE_STATS, -1);
    if (D->full_flag) {
      create_stats_queries(c, SPOLL_SEND_STATS | SPOLL_SEND_FULL_STATS, D->worker_pid);
    }

    D->need_end = 1;
    return_one_key_key(c, old_key);
    if (c->pending_queries == 0) {
      D->need_end = 0;
      php_master_wakeup(c);
    }
    return 0;
  }
  return SKIP_ALL_BYTES;
}

int php_master_get_end(connection *c, int key_cnt __attribute__((unused))) {
  if (c->status != conn_wait_net) {
    write_out(&c->Out, "END\r\n", 5);
  }
  return 0;
}

int php_master_version(connection *c) {
  write_out(&c->Out, "VERSION " PHP_MASTER_VERSION"\r\n", 9 + sizeof(PHP_MASTER_VERSION));
  return 0;
}

int php_master_rpc_stats(const vk::optional<std::vector<std::string>> &sorted_filter_keys) {
  std::string res(64 * 1024, 0);
  stats_t stats;
  stats.type = STATS_TYPE_TL;
  stats.statsd_prefix = nullptr;
  sb_init(&stats.sb, &res[0], static_cast<int>(res.size()) - 2);
  prepare_common_stats(&stats);
  res.resize(stats.sb.pos);
  res += php_master_prepare_stats(true, -1);
  tl_store_stats(res.c_str(), 0, sorted_filter_keys);
  return 0;
}

struct WorkerStats {
  int total_workers_n{0};
  int running_workers_n{0};
  int paused_workers_n{0};
  int ready_for_accept_workers_n{0};

  static WorkerStats collect() {
    WorkerStats result;
    for (int i = 0; i < me_all_workers_n; i++) {
      worker_info_t *w = workers[i];
      if (!w->is_dying) {
        result.total_workers_n++;
        result.running_workers_n += w->stats->istats.is_running;
        result.paused_workers_n += w->stats->istats.is_wait_net;
        result.ready_for_accept_workers_n += w->stats->istats.is_ready_for_accept;
      }
    }
    return result;
  }
};

std::string get_master_stats_html() {
  const auto worker_stats = WorkerStats::collect();

  std::string html;
  char buf[100];
  sprintf(buf, "total_workers\t%d\n", worker_stats.total_workers_n);
  html += buf;
  sprintf(buf, "free_workers\t%d\n", worker_stats.total_workers_n - worker_stats.running_workers_n);
  html += buf;
  sprintf(buf, "working_workers\t%d\n", worker_stats.running_workers_n);
  html += buf;
  sprintf(buf, "working_but_waiting_workers\t%d\n", worker_stats.paused_workers_n);
  html += buf;
  for (int i = 1; i <= 3; ++i) {
    sprintf(buf, "running_workers_avg_%s\t%7.3lf\n", periods_desc[i], server_stats.misc[i].get_stat().running_workers_avg);
    html += buf;
  }

  return html;
}

static long long int instance_cache_memory_swaps_ok = 0;
static long long int instance_cache_memory_swaps_fail = 0;
static const double FULL_STATS_PERIOD = 5.0;

class QPSCalculator {
public:
  explicit QPSCalculator(double cooldown_period) :
    cooldown_period_(cooldown_period) {
  }

  void update(double time_point, const PhpWorkerStats &new_worker_stats) {
    if (!prev_worker_stats_.total_queries()) {
      prev_worker_stats_ = new_worker_stats;
      prev_time_ = time_point;
      return;
    }

    const auto delta_incoming_queries = new_worker_stats.total_queries() - prev_worker_stats_.total_queries();
    if (delta_incoming_queries > 0) {
      const auto delta_time = time_point - prev_time_;
      incoming_qps_ = static_cast<double>(delta_incoming_queries) / delta_time;
      const auto delta_outgoing_queries = new_worker_stats.total_script_queries() - prev_worker_stats_.total_script_queries();
      outgoing_qps_ = static_cast<double>(delta_outgoing_queries) / delta_time;
      prev_time_ = time_point;
      prev_worker_stats_ = new_worker_stats;
    } else if (time_point - prev_time_ >= cooldown_period_) {
      incoming_qps_ = 0;
      outgoing_qps_ = 0;
      prev_time_ = time_point;
    }
  }

  double get_incoming_qps() const { return incoming_qps_; }
  double get_outgoing_qps() const { return outgoing_qps_; }

private:
  double incoming_qps_{0};
  double outgoing_qps_{0};
  double prev_time_{0};
  PhpWorkerStats prev_worker_stats_;
  const double cooldown_period_{0};
};

STATS_PROVIDER_TAGGED(kphp_stats, 100, STATS_TAG_KPHP_SERVER) {
  if (engine_tag) {
    add_gauge_stat_long(stats, "kphp_version", atoll(engine_tag));
  }

  add_gauge_stat_long(stats, "uptime", get_uptime());

  const auto worker_stats = WorkerStats::collect();
  add_gauge_stat_long(stats, "workers.current.total", worker_stats.total_workers_n);
  add_gauge_stat_long(stats, "workers.current.working", worker_stats.running_workers_n);
  add_gauge_stat_long(stats, "workers.current.working_but_waiting", worker_stats.paused_workers_n);
  add_gauge_stat_long(stats, "workers.current.ready_for_accept", worker_stats.ready_for_accept_workers_n);

  add_gauge_stat_long(stats, "workers.total.started", tot_workers_started);
  add_gauge_stat_long(stats, "workers.total.dead", tot_workers_dead);
  add_gauge_stat_long(stats, "workers.total.strange_dead", tot_workers_strange_dead);
  add_gauge_stat_long(stats, "workers.total.killed", workers_killed);
  add_gauge_stat_long(stats, "workers.total.hung", workers_hung);
  add_gauge_stat_long(stats, "workers.total.terminated", workers_terminated);
  add_gauge_stat_long(stats, "workers.total.failed", workers_failed);

  const auto workers_stats = server_stats.misc[1].get_stat();
  add_gauge_stat_double(stats, "workers.running.avg_1m", workers_stats.running_workers_avg);
  add_gauge_stat_long(stats, "workers.running.max_1m", workers_stats.running_workers_max);

  const auto cpu_stats = server_stats.cpu[1].get_stat();
  add_gauge_stat_double(stats, "cpu.stime", cpu_stats.cpu_s_usage);
  add_gauge_stat_double(stats, "cpu.utime", cpu_stats.cpu_u_usage);

  instance_cache_get_memory_stats().write_stats_to(stats, "instance_cache");
  add_gauge_stat_long(stats, "instance_cache.memory.buffer_swaps_ok", instance_cache_memory_swaps_ok);
  add_gauge_stat_long(stats, "instance_cache.memory.buffer_swaps_fail", instance_cache_memory_swaps_fail);

  const auto &instance_cache_element_stats = instance_cache_get_stats();
  add_gauge_stat_long(stats, "instance_cache.elements.stored",
                          instance_cache_element_stats.elements_stored.load(std::memory_order_relaxed));
  add_gauge_stat_long(stats, "instance_cache.elements.stored_with_delay",
                          instance_cache_element_stats.elements_stored_with_delay.load(std::memory_order_relaxed));
  add_gauge_stat_long(stats, "instance_cache.elements.storing_skipped_due_recent_update",
                          instance_cache_element_stats.elements_storing_skipped_due_recent_update.load(std::memory_order_relaxed));
  add_gauge_stat_long(stats, "instance_cache.elements.storing_delayed_due_mutex",
                          instance_cache_element_stats.elements_storing_delayed_due_mutex.load(std::memory_order_relaxed));
  add_gauge_stat_long(stats, "instance_cache.elements.fetched",
                          instance_cache_element_stats.elements_fetched.load(std::memory_order_relaxed));
  add_gauge_stat_long(stats, "instance_cache.elements.missed",
                          instance_cache_element_stats.elements_missed.load(std::memory_order_relaxed));
  add_gauge_stat_long(stats, "instance_cache.elements.missed_earlier",
                          instance_cache_element_stats.elements_missed_earlier.load(std::memory_order_relaxed));
  add_gauge_stat_long(stats, "instance_cache.elements.expired",
                          instance_cache_element_stats.elements_expired.load(std::memory_order_relaxed));
  add_gauge_stat_long(stats, "instance_cache.elements.created",
                          instance_cache_element_stats.elements_created.load(std::memory_order_relaxed));
  add_gauge_stat_long(stats, "instance_cache.elements.destroyed",
                          instance_cache_element_stats.elements_destroyed.load(std::memory_order_relaxed));
  add_gauge_stat_long(stats, "instance_cache.elements.cached",
                          instance_cache_element_stats.elements_cached.load(std::memory_order_relaxed));
  add_gauge_stat_long(stats, "instance_cache.elements.logically_expired_and_ignored",
                          instance_cache_element_stats.elements_logically_expired_and_ignored.load(std::memory_order_relaxed));
  add_gauge_stat_long(stats, "instance_cache.elements.logically_expired_but_fetched",
                          instance_cache_element_stats.elements_logically_expired_but_fetched.load(std::memory_order_relaxed));

  write_confdata_stats_to(stats);
  server_stats.worker_stats.recalc_master_percentiles();
  server_stats.worker_stats.to_stats(stats);

  static QPSCalculator qps_calculator{FULL_STATS_PERIOD * 2};
  qps_calculator.update(my_now, server_stats.worker_stats);
  add_gauge_stat_double(stats, "requests.incoming_queries_per_second", qps_calculator.get_incoming_qps());
  add_gauge_stat_double(stats, "requests.outgoing_queries_per_second", qps_calculator.get_outgoing_qps());

  add_gauge_stat_long(stats, "graceful_restart.warmup.final_new_instance_cache_size", WarmUpContext::get().get_final_new_instance_cache_size());
  add_gauge_stat_long(stats, "graceful_restart.warmup.final_old_instance_cache_size", WarmUpContext::get().get_final_old_instance_cache_size());

  const auto &job_workers_stats = job_workers::JobStats::get();
  add_gauge_stat_long(stats, "job_workers.job_queue_size", job_workers_stats.job_queue_size.load(std::memory_order_relaxed));
  add_gauge_stat_long(stats, "job_workers.currently_memory_slices_used", job_workers_stats.currently_memory_slices_used.load(std::memory_order_relaxed));
  add_gauge_stat_long(stats, "job_workers.max_shared_memory_slices_count", vk::singleton<job_workers::SharedMemoryManager>::get().get_total_slices_count());
  add_gauge_stat_long(stats, "job_workers.jobs_sent", job_workers_stats.jobs_sent.load(std::memory_order_relaxed));
  add_gauge_stat_long(stats, "job_workers.jobs_replied", job_workers_stats.jobs_replied.load(std::memory_order_relaxed));

  add_gauge_stat_long(stats, "job_workers.errors_pipe_server_write", job_workers_stats.errors_pipe_server_write.load(std::memory_order_relaxed));
  add_gauge_stat_long(stats, "job_workers.errors_pipe_server_read", job_workers_stats.errors_pipe_server_read.load(std::memory_order_relaxed));
  add_gauge_stat_long(stats, "job_workers.errors_pipe_client_write", job_workers_stats.errors_pipe_client_write.load(std::memory_order_relaxed));
  add_gauge_stat_long(stats, "job_workers.errors_pipe_client_read", job_workers_stats.errors_pipe_client_read.load(std::memory_order_relaxed));

  add_gauge_stat_long(stats, "job_workers.job_worker_skip_job_due_another_is_running",
                          job_workers_stats.job_worker_skip_job_due_another_is_running.load(std::memory_order_relaxed));
  add_gauge_stat_long(stats, "job_workers.job_worker_skip_job_due_overload",
                          job_workers_stats.job_worker_skip_job_due_overload.load(std::memory_order_relaxed));
  add_gauge_stat_long(stats, "job_workers.job_worker_skip_job_due_steal",
                          job_workers_stats.job_worker_skip_job_due_steal.load(std::memory_order_relaxed));

  update_mem_stats();
  unsigned long long max_vms = 0;
  unsigned long long max_rss = 0;
  unsigned long long max_shared = 0;
  for (int i = 0; i < me_all_workers_n; i++) {
    worker_info_t *w = workers[i];
    if (!w->is_dying) {
      const mem_info_t &mem_stats = w->stats->mem_info;
      max_vms = std::max(max_vms, mem_stats.vm_peak);
      max_rss = std::max(max_rss, mem_stats.rss_peak);
      max_shared = std::max(max_shared, mem_stats.rss_shmem + mem_stats.rss_file);
    }
  }

  add_gauge_stat_long(stats, "memory.vms_max", max_vms * 1024);
  add_gauge_stat_long(stats, "memory.rss_max", max_rss * 1024);
  add_gauge_stat_long(stats, "memory.shared_max", max_shared * 1024);
}

int php_master_http_execute(struct connection *c, int op) {
  struct hts_data *D = HTS_DATA(c);
  char ReqHdr[MAX_HTTP_HEADER_SIZE];

  vkprintf(1, "in php_master_http_execute: connection #%d, op=%d, header_size=%d, data_size=%d, http_version=%d\n",
           c->fd, op, D->header_size, D->data_size, D->http_ver);

  if (D->query_type != htqt_get) {
    D->query_flags &= ~QF_KEEPALIVE;
    return -501;
  }

  assert (D->header_size <= MAX_HTTP_HEADER_SIZE);
  assert (read_in(&c->In, &ReqHdr, D->header_size) == D->header_size);

  assert (D->first_line_size > 0 && D->first_line_size <= D->header_size);

  vkprintf(1, "===============\n%.*s\n==============\n", D->header_size, ReqHdr);
  vkprintf(1, "%d,%d,%d,%d\n", D->host_offset, D->host_size, D->uri_offset, D->uri_size);

  vkprintf(1, "hostname: '%.*s'\n", D->host_size, ReqHdr + D->host_offset);
  vkprintf(1, "URI: '%.*s'\n", D->uri_size, ReqHdr + D->uri_offset);

  const char *allowed_query = "/server-status";
  if (D->uri_size == strlen(allowed_query) && strncmp(ReqHdr + D->uri_offset, allowed_query, static_cast<size_t>(D->uri_size)) == 0) {
    std::string stat_html = get_master_stats_html();
    write_basic_http_header(c, 200, 0, static_cast<int>(stat_html.length()), nullptr, "text/plain; charset=UTF-8");
    write_out(&c->Out, stat_html.c_str(), static_cast<int>(stat_html.length()));
    return 0;
  }

  D->query_flags |= QF_ERROR;
  return -404;
}


/*** Main loop functions ***/

void run_master_off_in_graceful_shutdown() {
  vkprintf(2, "state: master_state::off_in_graceful_shutdown\n");
  assert(state == master_state::off_in_graceful_shutdown);
  to_kill = me_running_http_workers_n;
  if (me_running_http_workers_n + me_dying_http_workers_n == 0) {
    to_exit = 1;
    me->is_alive = false;
  }
}

void run_master_off_in_graceful_restart() {
  vkprintf(2, "state: master_state::off_in_graceful_restart\n");
  assert (other->is_alive);
  vkprintf(2, "other->to_kill_generation > me->generation --- %lld > %lld\n", other->to_kill_generation, me->generation);

  if (other->is_alive && other->ask_http_fd_generation > me->generation) {
    vkprintf(1, "send http fd\n");
    send_fd_via_socket(*http_fd);
    //TODO: process errors
    me->sent_http_fd_generation = static_cast<int>(generation);
    changed = 1;
  }

  if (other->to_kill_generation > me->generation) {
    // old master kills as many workers as new master told
    to_kill = other->to_kill;
  }

  if (me_running_http_workers_n + me_dying_http_workers_n == 0) {
    to_exit = 1;
    changed = 1;
    me->is_alive = false;
  }
}

void run_master_on() {
  vkprintf(2, "state: master_state::on\n");

  static double prev_attempt = 0;
  if (!master_sfd_inited && !other->is_alive && prev_attempt + 1 < my_now) {
    prev_attempt = my_now;
    if (master_port > 0 && master_sfd < 0) {
      master_sfd = server_socket(master_port, settings_addr, backlog, 0);
      if (master_sfd < 0) {
        static int failed_cnt = 0;

        failed_cnt++;
        if (failed_cnt > 2000) {
          vkprintf(-1, "cannot open master server socket at port %d: %m\n", master_port);
          exit(1);
        }
      } else {
        PID.port = (short)master_port;
        tl_stat_function = php_master_rpc_stats;
        init_listening_connection(master_sfd, &ct_tcp_rpc_server, &php_rpc_master_methods);
        master_sfd_inited = 1;
      }
    }
  }

  if (vk::singleton<JobWorkersContext>::get().job_workers_num > 0) {
    vk::singleton<JobWorkersContext>::get().master_init_pipes(workers_n);
  }

  bool need_http_fd = http_fd != nullptr && *http_fd == -1;

  if (need_http_fd) {
    int can_ask_http_fd = other->is_alive && other->own_http_fd && other->http_fd_port == me->http_fd_port;
    if (!can_ask_http_fd) {
      vkprintf(1, "Get http_fd via try_get_http_fd()\n");
      assert (try_get_http_fd != nullptr && "no pointer for try_get_http_fd found");
      *http_fd = try_get_http_fd();
      assert (*http_fd != -1 && "failed to get http_fd");
      me->own_http_fd = 1;
      need_http_fd = false;
    } else {
      if (me->ask_http_fd_generation != 0 && other->sent_http_fd_generation > me->generation) {
        vkprintf(1, "read http fd\n");
        *http_fd = receive_fd(socket_fd);
        vkprintf(1, "http_fd = %d\n", *http_fd);

        if (*http_fd == -1) {
          vkprintf(1, "wait for a second...\n");
          sleep(1);
          *http_fd = receive_fd(socket_fd);
          vkprintf(1, "http_fd = %d\n", *http_fd);
        }


        if (*http_fd != -1) {
          me->own_http_fd = 1;
          need_http_fd = false;
        } else {
          failed = 1;
        }
      } else {
        dl_assert (receive_fd_attempts_cnt < 4, dl_pstr("failed to receive http_fd: %d attempts are done\n", receive_fd_attempts_cnt));
        receive_fd_attempts_cnt++;

        vkprintf(1, "ask for http_fd\n");
        if (socket_fd != -1) {
          close(socket_fd);
        }
        socket_fd = sock_dgram(socket_name.c_str());
        me->ask_http_fd_generation = static_cast<int>(generation);
        changed = 1;
      }
    }
  }

  if (!need_http_fd) {
    int total_workers = me_running_http_workers_n + me_dying_http_workers_n + (other->is_alive ? other->running_http_workers_n + other->dying_http_workers_n : 0);
    to_run = std::max(0, workers_n - total_workers);

    if (other->is_alive) {
      auto &warm_up_ctx = WarmUpContext::get();
      warm_up_ctx.try_start_warmup();

      int set_to_kill = vk::clamp(MAX_KILL - other->dying_http_workers_n, 0, other->running_http_workers_n);
      bool need_more_workers_for_warmup = warm_up_ctx.need_more_workers_for_warmup();
      bool is_instance_cache_hot_enough = warm_up_ctx.is_instance_cache_hot_enough();
      bool warmup_timeout_expired       = warm_up_ctx.warmup_timeout_expired();
      if (set_to_kill > 0 && (need_more_workers_for_warmup || is_instance_cache_hot_enough || warmup_timeout_expired)) {
        // new master tells to old master how many workers it must kill
        vkprintf(1, "[set_to_kill = %d] [need_more_workers_for_warmup = %d] [is_instance_cache_hot_enough = %d] [new_instance_cache_size / old_instance_cache_size = %u / %u] [warmup_timeout_expired = %d]\n",
                        set_to_kill, need_more_workers_for_warmup,
                        is_instance_cache_hot_enough, me->instance_cache_elements_cached, other->instance_cache_elements_cached,
                        warmup_timeout_expired);
        if (!need_more_workers_for_warmup && (is_instance_cache_hot_enough || warmup_timeout_expired)) {
          warm_up_ctx.update_final_instance_cache_sizes();
        }
        me->to_kill = set_to_kill;
        me->to_kill_generation = generation;
        changed = 1;
      }
    }
  }
}

int signal_epoll_handler(int fd __attribute__((unused)), void *data __attribute__((unused)), event_t *ev __attribute__((unused))) {
  //empty
  vkprintf(2, "signal_epoll_handler\n");
  signalfd_siginfo fdsi{};
  //fprintf (stderr, "A\n");
  int s = (int)read(signal_fd, &fdsi, sizeof(signalfd_siginfo));
  //fprintf (stderr, "B\n");
  if (s == -1) {
    if (false && errno == EAGAIN) {
      vkprintf(1, "strange... no signal found\n");
      return 0;
    }
    dl_passert (0, "read signalfd_siginfo");
  }
  dl_assert (s == sizeof(signalfd_siginfo), dl_pstr("got %d bytes of %d expected", s, (int)sizeof(signalfd_siginfo)));
  vkprintf(2, "signal %u received\n", fdsi.ssi_signo);
  if (fdsi.ssi_signo == SIGTERM && !in_sigterm) {
    const char *message = "master got SIGTERM, starting graceful shutdown.\n";
    kwrite(2, message, strlen(message));
    in_sigterm = true;
  }
  return 0;
}

int update_mem_stats() {
  get_mem_stats(me->pid, &server_stats.mem_info);
  for (int i = 0; i < me_all_workers_n; i++) {
    worker_info_t *w = workers[i];

    if (get_mem_stats(w->pid, &w->stats->mem_info) != 1) {
      continue;
    }
    mem_info_add(&server_stats.mem_info, w->stats->mem_info);
  }
  return 0;
}

void check_and_instance_cache_try_swap_memory() {
  switch(instance_cache_try_swap_memory()) {
    case InstanceCacheSwapStatus::no_need:
      return;
    case InstanceCacheSwapStatus::swap_is_finished:
      vkprintf(0, "instance cache memory resource successfully swapped\n");
      ++instance_cache_memory_swaps_ok;
      return;
    case InstanceCacheSwapStatus::swap_is_forbidden:
      vkprintf(0, "can't swap instance cache memory resource\n");
      ++instance_cache_memory_swaps_fail;
      return;
  }
}

static void cron() {
  if (!other->is_alive || in_old_master_on_restart()) {
    // write stats at the beginning to avoid spikes in graphs
    send_data_to_statsd_with_prefix("kphp_server", STATS_TAG_KPHP_SERVER);
  }
  create_all_outbound_connections();

  unsigned long long cpu_total = 0;
  unsigned long long utime = 0;
  unsigned long long stime = 0;
#if !defined(__APPLE__)
  const bool get_cpu_err = get_cpu_total(&cpu_total);
  dl_assert (get_cpu_err, "get_cpu_total failed");
#endif
  server_stats.worker_stats.copy_internal_from(dead_worker_stats);
  server_stats.worker_stats.reset_memory_and_percentiles_stats();
  int running_workers = 0;
  for (int i = 0; i < me_all_workers_n; i++) {
    worker_info_t *w = workers[i];
    const bool get_pid_info_err = get_pid_info(w->pid, &w->my_info);
    w->valid_my_info = 1;
    if (!get_pid_info_err) {
      continue;
    }

    CpuStatTimestamp cpu_timestamp{my_now, w->my_info.utime, w->my_info.stime, cpu_total};
    utime += w->my_info.utime;
    stime += w->my_info.stime;
    w->stats->update(cpu_timestamp);
    running_workers += w->stats->istats.is_running;
    server_stats.worker_stats.add_from(w->stats->worker_stats);
  }
  MiscStatTimestamp misc_timestamp{my_now, running_workers};
  server_stats.update(misc_timestamp);

  utime += dead_utime;
  stime += dead_stime;
  CpuStatTimestamp cpu_timestamp{my_now, utime, stime, cpu_total};
  server_stats.update(cpu_timestamp);

  create_stats_queries(nullptr, SPOLL_SEND_STATS | SPOLL_SEND_IMMEDIATE_STATS, -1);
  static double last_full_stats = -1;
  if (last_full_stats + FULL_STATS_PERIOD < my_now) {
    last_full_stats = my_now;
    create_stats_queries(nullptr, SPOLL_SEND_STATS | SPOLL_SEND_FULL_STATS, -1);
  }

  instance_cache_purge_expired_elements();
  check_and_instance_cache_try_swap_memory();
  confdata_binlog_update_cron();
}

auto get_steady_tp_ms_now() noexcept {
  return std::chrono::time_point_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now());
}

static master_state master_change_state() {
  master_state new_state;
  if (in_sigterm) {
    new_state = master_state::off_in_graceful_shutdown;
  } else {
    if (!other->is_alive || in_new_master_on_restart()) {
      new_state = master_state::on;
    } else {
      new_state = master_state::off_in_graceful_restart;
    }
  }

  master_state prev_state = state;
  switch (state) {
    case master_state::on: {
      // can go to any state
      state = new_state;
      break;
    }
    case master_state::off_in_graceful_restart: {
      // can go to master_state::on if new master is crashed during graceful restart
      if (new_state == master_state::on) {
        state = new_state;
      }
      break;
    }
    case master_state::off_in_graceful_shutdown:
      // FINAL state: can't go to any other state
      break;
  }
  return prev_state;
}

void run_master() {
  cpu_cnt = (int)sysconf(_SC_NPROCESSORS_ONLN);
  me->http_fd_port = http_fd_port;
  me->own_http_fd = http_fd != nullptr && *http_fd != -1;

  epoll_sethandler(signal_fd, 0, signal_epoll_handler, nullptr);
  const int err = epoll_insert(signal_fd, EVT_READ);
  dl_assert (err >= 0, "epoll_insert failed");

  preallocate_msg_buffers();

  auto prev_cron_start_tp = get_steady_tp_ms_now();
  WarmUpContext::get().reset();
  while (true) {
    vkprintf(2, "run_master iteration: begin\n");
    tvkprintf(job_workers, 3, "Job queue size = %d\n", job_workers::JobStats::get().job_queue_size.load(std::memory_order_relaxed));

    my_now = dl_time();

    changed = 0;
    failed = 0;
    to_kill = 0;
    to_run = 0;
    to_exit = 0;

    update_workers();

    shared_data_lock(shared_data);
    shared_data_update(shared_data);

    //calc state
    dl_assert (me->is_alive && me->pid == getpid(), dl_pstr("[me->is_alive = %d] [me->pid = %d] [getpid() = %d]",
                                                              me->is_alive, me->pid, getpid()));
    master_state prev_state = master_change_state();

    if (state != prev_state && state == master_state::on) {
      WarmUpContext::get().reset();
    }

    //calc generation
    generation = me->generation;
    if (other->is_alive && other->generation > generation) {
      generation = other->generation;
    }
    generation++;

    switch (state) {
      case master_state::on:
        run_master_on();
        break;
      case master_state::off_in_graceful_restart:
        run_master_off_in_graceful_restart();
        break;
      case master_state::off_in_graceful_shutdown:
        run_master_off_in_graceful_shutdown();
        break;
      default:
        dl_unreachable ("unknown master state\n");
    }

    me->generation = generation;

    const auto &job_workers_ctx = vk::singleton<JobWorkersContext>::get();
    if (job_workers_ctx.job_workers_num > 0) {
      for (int i = 0; i < job_workers_ctx.job_workers_num - (job_workers_ctx.running_job_workers + job_workers_ctx.dying_job_workers); ++i) {
        if (run_worker(WorkerType::job_worker)) {
          tvkprintf(job_workers, 1, "launched new job worker with pid = %d\n", pid);
          return;
        }
      }
    }

    if (to_kill != 0 || to_run != 0) {
      vkprintf(1, "[to_kill = %d] [to_run = %d]\n", to_kill, to_run);
    }
    while (to_kill-- > 0) {
      kill_http_worker();
    }
    while (to_run-- > 0 && !failed) {
      if (run_worker(WorkerType::http_worker)) {
        return;
      }
    }
    kill_hanging_workers();

    me->running_http_workers_n = me_running_http_workers_n;
    me->dying_http_workers_n = me_dying_http_workers_n;
    me->instance_cache_elements_cached = static_cast<uint32_t>(instance_cache_get_stats().elements_cached.load(std::memory_order_relaxed));

    if (state != master_state::off_in_graceful_shutdown) {
      if (changed && other->is_alive) {
        vkprintf(1, "wakeup other master [pid = %d]\n", (int)other->pid);
        kill(other->pid, SIGPOLL);
      }
    }

    shared_data_unlock(shared_data);

    if (to_exit) {
      vkprintf(1, "all HTTP workers killed. Exit\n");
      _exit(0);
    }

    if (local_pending_signals & (1ll << SIGUSR1)) {
      local_pending_signals = local_pending_signals & ~(1ll << SIGUSR1);

      reopen_logs();
      reopen_json_log();
      workers_send_signal(SIGUSR1);
    }

    vkprintf(2, "run_master iteration: end\n");

    using namespace std::chrono_literals;
    auto wait_time = 1s - (get_steady_tp_ms_now() - prev_cron_start_tp);
    epoll_work(static_cast<int>(std::max(wait_time, 0ms).count()));

    const auto new_tp = get_steady_tp_ms_now();
    if (new_tp - prev_cron_start_tp >= 1s) {
      prev_cron_start_tp = new_tp;
      cron();
    }
  }
}
