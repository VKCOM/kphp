// Compiler for PHP (aka KPHP)
// Copyright (c) 2021 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "server/php-master.h"

#include <algorithm>
#include <cassert>
#include <cerrno>
#include <climits>
#include <cstdio>
#include <cstdlib>
#include <fcntl.h>
#include <iomanip>
#include <optional>
#include <poll.h>
#include <semaphore.h>
#include <sstream>
#include <stack>
#include <string>
#include <sys/mman.h>
#include <sys/signalfd.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/un.h>
#include <sys/wait.h>
#include <type_traits>
#include <unistd.h>
#include <vector>

#include "common/algorithms/find.h"
#include "common/crc32c.h"
#include "common/dl-utils-lite.h"
#include "common/kprintf.h"
#include "common/macos-ports.h"
#include "common/precise-time.h"
#include "common/server/tl-stats-t.h"
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
#include "server/confdata-binlog-replay.h"
#include "server/http-server-context.h"
#include "server/lease-rpc-client.h"
#include "server/master-name.h"
#include "server/numa-configuration.h"
#include "server/php-engine-vars.h"
#include "server/php-engine.h"
#include "server/php-master-tl-handlers.h"
#include "server/server-stats.h"
#include "server/shared-data-worker-cache.h"
#include "server/shared-data.h"
#include "server/statshouse/add-metrics-batch.h"
#include "server/statshouse/statshouse-client.h"
#include "server/workers-control.h"

#include "server/php-master-restart.h"
#include "server/php-master-warmup.h"
#include "server/server-log.h"

#include "server/job-workers/job-stats.h"
#include "server/job-workers/job-worker-client.h"
#include "server/job-workers/job-workers-context.h"
#include "server/job-workers/shared-memory-manager.h"
#include "server/json-logger.h"
#include "server/server-config.h"

using job_workers::JobWorkersContext;

extern const char *engine_tag;

//do not kill more then MAX_KILL at the same time
#define MAX_KILL 5

#define PHP_MASTER_VERSION "0.1"

DEFINE_VERBOSITY(graceful_restart)

static int cpu_cnt;

static sigset_t mask;
static sigset_t orig_mask;
static sigset_t empty_mask;

static double my_now;

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
  std::string cpu_desc, misc_desc;
  StatImpl<CpuStatSegment, CpuStatTimestamp> cpu[periods_n];
  StatImpl<MiscStatSegment, MiscStatTimestamp> misc_stat_for_general_workers[periods_n];
  StatImpl<MiscStatSegment, MiscStatTimestamp> misc_stat_for_job_workers[periods_n];

  Stats() {
    for (int i = 0; i < periods_n; i++) {
      cpu[i].set_period(periods_len[i]);
      misc_stat_for_general_workers[i].set_period(periods_len[i]);
      misc_stat_for_job_workers[i].set_period(periods_len[i]);
    }

    for (const auto *period : periods_desc) {
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

  void update_misc_stat_for_general_workers(const MiscStatTimestamp &misc_timestamp) {
    for (int i = 1; i < periods_n; i++) {
      misc_stat_for_general_workers[i].add_timestamp(misc_timestamp);
    }
  }

  void update_misc_stat_for_job_workers(const MiscStatTimestamp &misc_timestamp) {
    for (int i = 1; i < periods_n; i++) {
      misc_stat_for_job_workers[i].add_timestamp(misc_timestamp);
    }
  }

  void write_to(std::ostream &out, int proc_pid) {
    out << "cpu_usage";
    if (proc_pid) {
      out << " " << proc_pid;
    }
    out << std::fixed << std::setprecision(2) << "(" << cpu_desc << ")\t";
    for (auto &i_cpu : cpu) {
      out << " " << cpu_cnt * (i_cpu.get_stat().cpu_usage * 100) << "%";
    }
    out << "\n";

    if (!proc_pid) {
      out << std::fixed << std::setprecision(3) << "running_workers_avg(" << misc_desc << ")\t";
      for (int i = 1; i < periods_n; i++) {
        out << " " << misc_stat_for_general_workers[i].get_stat().running_workers_avg;
      }
      out << "\nrunning_workers_max(" << misc_desc << ")\t";
      for (int i = 1; i < periods_n; i++) {
        out << " " << misc_stat_for_general_workers[i].get_stat().running_workers_max;
      }
      out << "\n";
    }
  }
};

WorkerType run_master();

namespace {
volatile long long local_pending_signals = 0;

void sigusr1_handler(const int sig) {
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
enum class master_state { on, off_in_graceful_restart, off_in_graceful_shutdown };

struct worker_info_t {
  worker_info_t *next_worker;

  int generation;
  pid_t pid;
  int is_dying;

  uint64_t last_activity_counter;
  double last_activity_time;
  double start_time;
  double kill_time;
  int kill_flag;

  pid_info_t my_info;
  int valid_my_info;

  int unique_id;

  Stats *stats;
  WorkerType type;
};

worker_info_t *workers[WorkersControl::max_workers_count];
Stats server_stats;
unsigned long long dead_stime, dead_utime;

master_state state;
bool in_sigterm;

int signal_fd;

int changed = 0;
int failed = 0;
int socket_fd = -1;
int to_kill = 0, to_run = 0, to_exit = 0;
int job_workers_to_kill = 0, job_workers_to_run = 0;
long long generation;
int receive_fd_attempts_cnt = 0;

worker_info_t *free_workers = nullptr;

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
  if (w->valid_my_info) {
    dead_utime += w->my_info.utime;
    dead_stime += w->my_info.stime;
  }
  worker_free(w);
  w->next_worker = free_workers;
  free_workers = w;
}

void terminate_worker(worker_info_t *w) {
  vkprintf(1, "kill_worker: send SIGTERM to [pid = %d]\n", (int)w->pid);
  kill(w->pid, SIGTERM);
  w->is_dying = 1;
  w->kill_time = my_now + 35;
  w->kill_flag = 0;

  vk::singleton<WorkersControl>::get().on_worker_terminating(w->type);
  if (w->type == WorkerType::general_worker) {
    changed = 1;
  }
  workers_terminated++;
}

int kill_worker(WorkerType worker_type) {
  for (int i = 0; i < vk::singleton<WorkersControl>::get().get_all_alive(); i++) {
    if (workers[i]->type == worker_type && !workers[i]->is_dying) {
      terminate_worker(workers[i]);
      return 1;
    }
  }
  return 0;
}

static int get_max_hanging_time_sec() noexcept {
  return max(script_timeout + 1, 65); // + 1 sec for terminating
}

void kill_hanging_workers() {
  static double last_terminated = -1;
  if (last_terminated + 30 < my_now) {
    for (int i = 0; i < vk::singleton<WorkersControl>::get().get_all_alive(); i++) {
      auto *worker = workers[i];
      const auto worker_activity_counter = vk::singleton<ServerStats>::get().get_worker_activity_counter(worker->unique_id);
      if (worker_activity_counter != worker->last_activity_counter) {
        worker->last_activity_counter = worker_activity_counter;
        worker->last_activity_time = my_now;
        continue;
      }
      if (!worker->is_dying && worker->last_activity_time + get_max_hanging_time_sec() <= my_now) {
        vkprintf(1, "No stats received from worker [pid = %d]. Terminate it\n", static_cast<int>(worker->pid));
        if (workers[i]->type == WorkerType::job_worker) {
          tvkprintf(job_workers, 1, "No stats received from job worker [pid = %d]. Terminate it\n", static_cast<int>(worker->pid));
        }
        workers_hung++;
        terminate_worker(worker);
        last_terminated = my_now;
        break;
      }
    }
  }

  for (int i = 0; i < vk::singleton<WorkersControl>::get().get_all_alive(); i++) {
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
  for (int i = 0; i < vk::singleton<WorkersControl>::get().get_all_alive(); i++) {
    if (!workers[i]->is_dying) {
      kill(workers[i]->pid, sig);
    }
  }
}

bool all_http_workers_killed() {
  return vk::singleton<WorkersControl>::get().get_alive_count(WorkerType::general_worker) == 0;
}

bool all_job_workers_killed() {
  return vk::singleton<WorkersControl>::get().get_alive_count(WorkerType::job_worker) == 0;
}

} // namespace

WorkerType start_master() {
  initial_verbosity = verbosity;

  kprintf("[master_name=%s] [cluster_name=%s] [shmem_name=%s] [socket_name=%s]\n",
          vk::singleton<MasterName>::get().get_master_name(), vk::singleton<ServerConfig>::get().get_cluster_name(),
          vk::singleton<MasterName>::get().get_shmem_name(), vk::singleton<MasterName>::get().get_socket_name());

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
    shared_data = get_shared_data(vk::singleton<MasterName>::get().get_shmem_name());
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

  return run_master();
}

int run_worker(WorkerType worker_type) {
  dl_block_all_signals();

  assert (vk::singleton<WorkersControl>::get().get_all_alive() < WorkersControl::max_workers_count);

  tot_workers_started++;
  const uint16_t worker_unique_id = vk::singleton<WorkersControl>::get().on_worker_creating(worker_type);
  pid_t new_pid = fork();
  if (new_pid == -1) {
    log_server_critical("fork error on launching %s worker: %s", (worker_type == WorkerType::general_worker ? "general" : "job"), strerror(errno));
    assert(false);
  }
  assert (new_pid != -1 && "failed to fork");

  if (new_pid == 0) {
    prctl(PR_SET_PDEATHSIG, SIGKILL); // TODO: or SIGTERM
    if (getppid() != me->pid) {
      vkprintf(0, "parent is dead just after start\n");
      exit(123);
    }

    auto &numa = vk::singleton<NumaConfiguration>::get();
    if (numa.enabled()) {
      numa.distribute_worker(worker_unique_id);
    }

    vk::singleton<HttpServerContext>::get().dedicate_http_socket_to_worker(worker_unique_id);

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

    master_sfd = -1;
    if (worker_type == WorkerType::job_worker) {
      process_type = ProcessType::job_worker;
    } else if (RpcClients::get().rpc_clients.empty()) {
      process_type = ProcessType::http_worker;
    } else {
      process_type = ProcessType::rpc_worker;
    }

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
    logname_id = worker_unique_id;
    if (logname_pattern) {
      char buf[100];
      snprintf(buf, 100, logname_pattern, worker_unique_id);
      logname = strdup(buf);
    }

    instance_cache_release_all_resources_acquired_by_this_proc();
    ConfdataGlobalManager::get().force_release_all_resources_acquired_by_this_proc_if_init();
    vk::singleton<job_workers::SharedMemoryManager>::get().forcibly_release_all_attached_messages();
    vk::singleton<ServerStats>::get().after_fork(pid, active_special_connections, max_special_connections, worker_unique_id, worker_type);
    return 1;
  }

  dl_restore_signal_mask();

  vkprintf(1, "new worker launched [pid = %d]\n", (int)new_pid);

  worker_info_t *worker = workers[vk::singleton<WorkersControl>::get().get_all_alive() - 1] = new_worker();
  worker->pid = new_pid;

  worker->is_dying = 0;
  worker->generation = ++conn_generation;
  worker->start_time = my_now;
  worker->unique_id = worker_unique_id;
  worker->last_activity_counter = 0;
  worker->last_activity_time = my_now;
  worker->type = worker_type;

  changed = 1;

  return 0;
}

void remove_worker(pid_t pid) {
  vkprintf(2, "remove workers [pid = %d]\n", static_cast<int>(pid));
  const auto &workers_control = vk::singleton<WorkersControl>::get();
  for (int i = 0; i < workers_control.get_all_alive(); i++) {
    if (workers[i]->pid == pid) {
      vk::singleton<WorkersControl>::get().on_worker_removing(workers[i]->type, workers[i]->is_dying, workers[i]->unique_id);
      if (workers[i]->type == WorkerType::general_worker && !workers[i]->is_dying) {
        failed++;
      }
      workers_failed++;

      delete_worker(workers[i]);

      workers[i] = workers[workers_control.get_all_alive()];
      vkprintf(1, "worker removed: [general running = %d] [general dying = %d]\n",
               int{workers_control.get_running_count(WorkerType::general_worker)},
               int{workers_control.get_dying_count(WorkerType::general_worker)});
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
  static bool inited = false;

  if (!inited) {
    init_sockaddr_un(&unix_socket_addr, vk::singleton<MasterName>::get().get_socket_name());
    inited = true;
  }

  return &unix_socket_addr;
}

static void send_fds_via_socket(const std::vector<int> &fds) {
  tvkprintf(graceful_restart, 1, "Graceful restart: sending http fds\n");

  int unix_socket_fd = socket(AF_LOCAL, SOCK_DGRAM, 0);
  dl_passert(!fds.empty(), "failed to create socket");

  const sockaddr_un *dest_addr = get_socket_addr();

  int iov[1] = { static_cast<int>(fds.size()) }; // must send/receive at least one byte, send fds number here
  iovec vec = {
    .iov_base = static_cast<void *>(iov),
    .iov_len = sizeof(int),
  };

  std::aligned_storage_t<CMSG_SPACE(HttpServerContext::MAX_HTTP_PORTS * sizeof(int)), alignof(cmsghdr)> buf;

  msghdr msg = {
    .msg_name = (void *)dest_addr,
    .msg_namelen = sizeof(*dest_addr),
    .msg_iov = &vec,
    .msg_iovlen = 1,
    .msg_control = &buf,
    .msg_controllen = sizeof(buf),
    .msg_flags = 0,
  };

  size_t payload_data_len = fds.size() * sizeof(int);

  cmsghdr *cmsg = CMSG_FIRSTHDR(&msg);
  cmsg->cmsg_level = SOL_SOCKET;
  cmsg->cmsg_type = SCM_RIGHTS;
  cmsg->cmsg_len = CMSG_LEN(payload_data_len);
  memcpy(CMSG_DATA(cmsg), fds.data(), payload_data_len);

  msg.msg_controllen = cmsg->cmsg_len;

  int rv = sendmsg(unix_socket_fd, &msg, 0);
  if (rv != -1) {
    tvkprintf(graceful_restart, 1, "Graceful restart: http fds sent successfully\n");
  } else {
    perror("failed to send http_fd (sendmsg)");
  }
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

/* receive file descriptors over unix socket */
static std::vector<int> receive_fds(int unix_socket_fd) {
  tvkprintf(graceful_restart, 1, "Graceful restart: receiving http fds from old master\n");

  std::aligned_storage_t<CMSG_SPACE(HttpServerContext::MAX_HTTP_PORTS * sizeof(int)), alignof(cmsghdr)> buf;

  int iobuf[1];
  iovec iov = {
    .iov_base = static_cast<void *>(iobuf),
    .iov_len = sizeof(int),
  };

  msghdr msg = {
    .msg_name = nullptr,
    .msg_namelen = 0,
    .msg_iov = &iov,
    .msg_iovlen = 1,
    .msg_control = &buf,
    .msg_controllen = sizeof(buf),
    .msg_flags = 0,
  };

  int rv = recvmsg(unix_socket_fd, &msg, 0);
  if (rv == -1) {
    perror("recvmsg");
    return {};
  }

  int http_fds_count = *static_cast<int *>(msg.msg_iov->iov_base);

  tvkprintf(graceful_restart, 1, "Graceful restart: http fds received successfully, got %d fds from old master\n", http_fds_count);

  cmsghdr *cmsg = CMSG_FIRSTHDR (&msg);
  if (cmsg->cmsg_type != SCM_RIGHTS) {
    kprintf("Got control message of unknown type %d\n", cmsg->cmsg_type);
    return {};
  }
  int *received_http_fds = reinterpret_cast<int *>(CMSG_DATA(cmsg));

  std::vector<int> fds;
  for (int i = 0; i < http_fds_count; ++i) {
    tvkprintf(graceful_restart, 1, "Graceful restart: got http fd %d\n", received_http_fds[i]);
    fds.emplace_back(received_http_fds[i]);
  }
  return fds;
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

int return_one_key_key(connection *c, const char *key) {
  std::string tmp;
  tmp += "VALUE ";
  tmp += key;
  write_out(&c->Out, tmp.c_str(), (int)tmp.size());
  return 0;
}

int return_one_key_val(connection *c, const char *val, int vlen) {
  const size_t tmp_size = 300;
  char tmp[tmp_size];
  int l = snprintf(tmp, tmp_size, " 0 %d\r\n", vlen);
  assert (l < 300);
  write_out(&c->Out, tmp, l);
  write_out(&c->Out, val, vlen);
  write_out(&c->Out, "\r\n", 2);
  return 0;
}

std::string php_master_prepare_stats(bool add_worker_pids) {
  auto *last_worker = workers + vk::singleton<WorkersControl>::get().get_all_alive();
  auto start = std::minmax_element(workers, last_worker, [](const worker_info_t *l, const worker_info_t *r) {
    return l->start_time < r->start_time;
  });
  auto min_uptime = start.second == last_worker ? 0 : static_cast<int64_t>(my_now - (*start.second)->start_time);
  auto max_uptime = start.first == last_worker ? 0 : static_cast<int64_t>(my_now - (*start.first)->start_time);

  const auto &stats = vk::singleton<ServerStats>::get();
  const auto general_workers_stat = stats.collect_workers_stat(WorkerType::general_worker);
  const auto job_workers_stat = stats.collect_workers_stat(WorkerType::job_worker);

  std::ostringstream oss;
  server_stats.write_to(oss, 0);
  oss << "uptime\t" << get_uptime() << "\n";
  if (engine_tag) {
    // engine_tag may be ended with "["
    oss << "kphp_version\t" << atoll(engine_tag) << "\n";
  }
  oss << "cluster_name\t" << vk::singleton<ServerConfig>::get().get_cluster_name() << "\n"
      << "master_name\t" << vk::singleton<MasterName>::get().get_master_name() << "\n"
      << "min_worker_uptime\t" << min_uptime << "\n"
      << "max_worker_uptime\t" << max_uptime << "\n"
      << "total_workers\t" << general_workers_stat.total_workers + job_workers_stat.total_workers << "\n"
      << "running_workers\t" << general_workers_stat.running_workers + job_workers_stat.running_workers << "\n"
      << "paused_workers\t" << general_workers_stat.waiting_workers + job_workers_stat.waiting_workers << "\n"
      << "tot_workers_started\t" << tot_workers_started << "\n"
      << "tot_workers_dead\t" << tot_workers_dead << "\n"
      << "tot_workers_strange_dead\t" << tot_workers_strange_dead << "\n"
      << "workers_killed\t" << workers_killed << "\n"
      << "workers_hung\t" << workers_hung << "\n"
      << "workers_terminated\t" << workers_terminated << "\n"
      << "workers_failed\t" << workers_failed << "\n";
  stats.write_stats_to(oss, add_worker_pids);

  std::for_each(workers, last_worker, [&oss](const worker_info_t *w) {
    oss << "worker_uptime " << w->pid << "\t" << static_cast<int64_t>(my_now - w->start_time) << "\n";
    w->stats->write_to(oss, w->pid);
  });
  return oss.str();
}


int php_master_wakeup(connection *c) {
  if (c->status == conn_wait_net) {
    c->status = conn_expect_query;
  }

  update_workers();

  std::string res = php_master_prepare_stats(false);
  return_one_key_val(c, res.c_str(), static_cast<int>(res.size()));

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

  if (key_len == 12 && strncmp(key, "workers_pids", 12) == 0) {
    std::string res;
    for (int i = 0; i < vk::singleton<WorkersControl>::get().get_all_alive(); i++) {
      if (!workers[i]->is_dying) {
        const size_t buf_size = 30;
        char buf[buf_size];
        snprintf(buf, buf_size, "%d", workers[i]->pid);
        if (!res.empty()) {
          res += ",";
        }
        res += buf;
      }
    }
    return_one_key(c, old_key, res.c_str(), static_cast<int>(res.size()));
    return 0;
  }
  if (key_len >= 5 && strncmp(key, "stats", 5) == 0) {
    return_one_key_key(c, old_key);
    php_master_wakeup(c);
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

int php_master_rpc_stats(const std::optional<std::vector<std::string>> &sorted_filter_keys) {
  std::string res(64 * 1024, 0);
  tl_stats_t stats;
  stats.stats_prefix = nullptr;
  sb_init(&stats.sb, &res[0], static_cast<int>(res.size()) - 2);
  prepare_common_stats(&stats);
  res.resize(stats.sb.pos);
  res += php_master_prepare_stats(true);
  tl_store_stats(res.c_str(), 0, sorted_filter_keys);
  return 0;
}

std::string get_master_stats_html() {
  const auto worker_stats = vk::singleton<ServerStats>::get().collect_workers_stat(WorkerType::general_worker);
  std::ostringstream html;
  html << "cluster_name\t" << vk::singleton<ServerConfig>::get().get_cluster_name() << "\n"
       << "master_name\t" << vk::singleton<MasterName>::get().get_master_name() << "\n"
       << "total_workers\t" << worker_stats.total_workers << "\n"
       << "free_workers\t" << worker_stats.total_workers - worker_stats.running_workers << "\n"
       << "working_workers\t" << worker_stats.running_workers << "\n"
       << "working_but_waiting_workers\t" << worker_stats.waiting_workers << "\n"
       << std::fixed << std::setprecision(3);
  for (int i = 1; i <= 3; ++i) {
    html << "running_workers_avg_" << periods_desc[i] << "\t" << server_stats.misc_stat_for_general_workers[i].get_stat().running_workers_avg << "\n";
  }
  return html.str();
}

static long long int instance_cache_memory_swaps_ok = 0;
static long long int instance_cache_memory_swaps_fail = 0;

STATS_PROVIDER_TAGGED(kphp_stats, 100, stats_tag_kphp_server) {
  if (engine_tag) {
    stats->add_gauge_stat("kphp_version", atoll(engine_tag));
  }

  stats->add_gauge_stat("uptime", get_uptime());

  const auto general_worker_group = vk::singleton<ServerStats>::get().collect_workers_stat(WorkerType::general_worker);
  stats->add_gauge_stat("workers.general.processes.total", general_worker_group.total_workers);
  stats->add_gauge_stat("workers.general.processes.working", general_worker_group.running_workers);
  stats->add_gauge_stat("workers.general.processes.working_but_waiting", general_worker_group.waiting_workers);
  stats->add_gauge_stat("workers.general.processes.ready_for_accept", general_worker_group.ready_for_accept_workers);

  const auto job_worker_group = vk::singleton<ServerStats>::get().collect_workers_stat(WorkerType::job_worker);
  stats->add_gauge_stat("workers.job.processes.total", job_worker_group.total_workers);
  stats->add_gauge_stat("workers.job.processes.working", job_worker_group.running_workers);
  stats->add_gauge_stat("workers.job.processes.working_but_waiting", job_worker_group.waiting_workers);

  if (stats->need_aggregated_stats()) {
    auto running_stats = server_stats.misc_stat_for_general_workers[1].get_stat();
    stats->add_gauge_stat("workers.general.processes.running.avg_1m", running_stats.running_workers_avg);
    stats->add_gauge_stat("workers.general.processes.running.max_1m", running_stats.running_workers_max);

    running_stats = server_stats.misc_stat_for_job_workers[1].get_stat();
    stats->add_gauge_stat("workers.job.processes.running.avg_1m", running_stats.running_workers_avg);
    stats->add_gauge_stat("workers.job.processes.running.max_1m", running_stats.running_workers_max);
  }

  stats->add_gauge_stat("server.workers.started", tot_workers_started);
  stats->add_gauge_stat("server.workers.dead", tot_workers_dead);
  stats->add_gauge_stat("server.workers.strange_dead", tot_workers_strange_dead);
  stats->add_gauge_stat("server.workers.killed", workers_killed);
  stats->add_gauge_stat("server.workers.hung", workers_hung);
  stats->add_gauge_stat("server.workers.terminated", workers_terminated);
  stats->add_gauge_stat("server.workers.failed", workers_failed);

  const auto cpu_stats = server_stats.cpu[1].get_stat();
  stats->add_gauge_stat("cpu.stime", cpu_stats.cpu_s_usage);
  stats->add_gauge_stat("cpu.utime", cpu_stats.cpu_u_usage);

  auto total_workers_json_count = vk::singleton<ServerStats>::get().collect_json_count_stat();
  uint64_t master_json_logs_count = vk::singleton<JsonLogger>::get().get_json_logs_count();
  stats->add_gauge_stat("server.total_json_logs_count", std::get<0>(total_workers_json_count) + master_json_logs_count);
  stats->add_gauge_stat("server.total_json_traces_count", std::get<1>(total_workers_json_count));

  instance_cache_get_memory_stats().write_stats_to(stats, "instance_cache");
  stats->add_gauge_stat("instance_cache.memory.buffer_swaps_ok", instance_cache_memory_swaps_ok);
  stats->add_gauge_stat("instance_cache.memory.buffer_swaps_fail", instance_cache_memory_swaps_fail);

  const auto &instance_cache_element_stats = instance_cache_get_stats();
  stats->add_gauge_stat(instance_cache_element_stats.elements_stored, "instance_cache.elements.stored");
  stats->add_gauge_stat(instance_cache_element_stats.elements_stored_with_delay, "instance_cache.elements.stored_with_delay");
  stats->add_gauge_stat(instance_cache_element_stats.elements_storing_skipped_due_recent_update, "instance_cache.elements.storing_skipped_due_recent_update");
  stats->add_gauge_stat(instance_cache_element_stats.elements_storing_delayed_due_mutex, "instance_cache.elements.storing_delayed_due_mutex");
  stats->add_gauge_stat(instance_cache_element_stats.elements_fetched, "instance_cache.elements.fetched");
  stats->add_gauge_stat(instance_cache_element_stats.elements_missed, "instance_cache.elements.missed");
  stats->add_gauge_stat(instance_cache_element_stats.elements_missed_earlier, "instance_cache.elements.missed_earlier");
  stats->add_gauge_stat(instance_cache_element_stats.elements_expired, "instance_cache.elements.expired");
  stats->add_gauge_stat(instance_cache_element_stats.elements_created, "instance_cache.elements.created");
  stats->add_gauge_stat(instance_cache_element_stats.elements_destroyed, "instance_cache.elements.destroyed");
  stats->add_gauge_stat(instance_cache_element_stats.elements_cached, "instance_cache.elements.cached");
  stats->add_gauge_stat(instance_cache_element_stats.elements_logically_expired_and_ignored, "instance_cache.elements.logically_expired_and_ignored");
  stats->add_gauge_stat(instance_cache_element_stats.elements_logically_expired_but_fetched, "instance_cache.elements.logically_expired_but_fetched");

  write_confdata_stats_to(stats);
  vk::singleton<ServerStats>::get().write_stats_to(stats);

  stats->add_gauge_stat("graceful_restart.warmup.final_new_instance_cache_size", WarmUpContext::get().get_final_new_instance_cache_size());
  stats->add_gauge_stat("graceful_restart.warmup.final_old_instance_cache_size", WarmUpContext::get().get_final_old_instance_cache_size());

  if (vk::singleton<job_workers::SharedMemoryManager>::get().is_initialized()) {
    vk::singleton<job_workers::SharedMemoryManager>::get().get_stats().write_stats_to(stats);
  }
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
  to_kill = vk::singleton<WorkersControl>::get().get_running_count(WorkerType::general_worker);
  if (all_http_workers_killed()) {
    if (all_job_workers_killed()) {
      to_exit = 1;
      me->is_alive = false;
    } else {
      job_workers_to_kill = vk::singleton<WorkersControl>::get().get_running_count(WorkerType::job_worker);
    }
  }
}

void run_master_off_in_graceful_restart() {
  vkprintf(2, "state: master_state::off_in_graceful_restart\n");
  assert (other->is_alive);
  vkprintf(2, "other->to_kill_generation > me->generation --- %lld > %lld\n", other->to_kill_generation, me->generation);

  if (other->is_alive && other->ask_http_fds_generation > me->generation) {
    send_fds_via_socket(vk::singleton<HttpServerContext>::get().http_socket_fds());
    //TODO: process errors
    me->sent_http_fds_generation = static_cast<int>(generation);
    changed = 1;
  }

  if (other->to_kill_generation > me->generation) {
    // old master kills as many workers as new master told
    to_kill = other->to_kill;
  }

  if (all_http_workers_killed()) {
    if (all_job_workers_killed()) {
      to_exit = 1;
      changed = 1;
      me->is_alive = false;
    } else {
      job_workers_to_kill = vk::singleton<WorkersControl>::get().get_running_count(WorkerType::job_worker);
    }
  }
}

bool init_http_sockets_if_needed() {
  static bool http_sockets_inited = false;

  auto &http_ctx = vk::singleton<HttpServerContext>::get();

  if (!http_ctx.http_server_enabled() || http_sockets_inited) {
    return true;
  }

  bool can_ask_http_fds = other->is_alive && other->own_http_fds &&
                          other->http_ports_count == me->http_ports_count && std::equal(me->http_ports, me->http_ports + me->http_ports_count, other->http_ports);
  if (!can_ask_http_fds) {
    vkprintf(1, "Create http sockets\n");

    bool ok = http_ctx.master_create_http_sockets();
    assert(ok && "failed to create HTTP sockets");
    me->own_http_fds = 1;
    http_sockets_inited = true;
  } else {
    tvkprintf(graceful_restart, 1, "Graceful restart: going to get http fds from old master\n");
    if (me->ask_http_fds_generation != 0 && other->sent_http_fds_generation > me->generation) {
      std::vector<int> open_http_sfds = receive_fds(socket_fd);

      if (open_http_sfds.empty()) {
        tvkprintf(graceful_restart, 1, "Graceful restart: failed to receive fds, wait for a second...\n");
        sleep(1);
        open_http_sfds = receive_fds(socket_fd);
      }

      if (!open_http_sfds.empty()) {
        http_ctx.master_set_open_http_sockets(std::move(open_http_sfds));
        me->own_http_fds = 1;
        http_sockets_inited = true;
      } else {
        failed = 1;
      }
    } else {
      dl_assert(receive_fd_attempts_cnt < 4, dl_pstr("failed to receive http_fd: %d attempts are done\n", receive_fd_attempts_cnt));
      receive_fd_attempts_cnt++;

      tvkprintf(graceful_restart, 1, "Graceful restart: asking for http fds, try#%d\n", receive_fd_attempts_cnt);
      if (socket_fd != -1) {
        close(socket_fd);
      }
      socket_fd = sock_dgram(vk::singleton<MasterName>::get().get_socket_name());
      me->ask_http_fds_generation = static_cast<int>(generation);
      changed = 1;
    }
  }
  return http_sockets_inited;
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

  if (vk::singleton<WorkersControl>::get().get_count(WorkerType::job_worker) > 0) {
    vk::singleton<JobWorkersContext>::get().master_init_pipes(vk::singleton<WorkersControl>::get().get_total_workers_count());
  }

  bool done = init_http_sockets_if_needed();

  if (done) {
    const auto &control = vk::singleton<WorkersControl>::get();
    const int total_workers = control.get_alive_count(WorkerType::general_worker) + (other->is_alive ? other->running_http_workers_n + other->dying_http_workers_n : 0);
    to_run = std::max(0, int{control.get_count(WorkerType::general_worker)} - total_workers);
    job_workers_to_run = control.get_count(WorkerType::job_worker) - control.get_alive_count(WorkerType::job_worker);

    if (other->is_alive) {
      auto &warm_up_ctx = WarmUpContext::get();
      warm_up_ctx.try_start_warmup();

      int set_to_kill = std::clamp(MAX_KILL - other->dying_http_workers_n, 0, other->running_http_workers_n);
      bool need_more_workers_for_warmup = warm_up_ctx.need_more_workers_for_warmup();
      bool is_instance_cache_hot_enough = warm_up_ctx.is_instance_cache_hot_enough();
      bool warmup_timeout_expired = warm_up_ctx.warmup_timeout_expired();
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

int signal_epoll_handler(int fd, void *data __attribute__((unused)), event_t *ev __attribute__((unused))) {
  vkprintf(2, "signal_epoll_handler\n");
  signalfd_siginfo fdsi{};
  int s = (int)read(signal_fd, &fdsi, sizeof(signalfd_siginfo));
  if (s == -1) {
    log_server_error("Master process can't read signal from signalfd: fd = %d, signal_fd = %d, error = %s", fd, signal_fd, strerror(errno));
    return 0;
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

void check_and_instance_cache_try_swap_memory() {
  switch(instance_cache_try_swap_memory()) {
    case InstanceCacheSwapStatus::no_need:
      return;
    case InstanceCacheSwapStatus::swap_is_finished:
      kprintf("instance cache memory resource successfully swapped\n");
      ++instance_cache_memory_swaps_ok;
      return;
    case InstanceCacheSwapStatus::swap_is_forbidden:
      log_server_error("can't swap instance cache memory resource\n");
      ++instance_cache_memory_swaps_fail;
      return;
  }
}

static void cron() {
  if (!other->is_alive || in_old_master_on_restart()) {
    // write stats at the beginning to avoid spikes in graphs
    send_data_to_statsd_with_prefix(vk::singleton<ServerConfig>::get().get_statsd_prefix(), stats_tag_kphp_server);
    vk::singleton<StatsHouseClient>::get().master_send_metrics();
  }
  create_all_outbound_connections();
  vk::singleton<ServerStats>::get().aggregate_stats();

  unsigned long long cpu_total = 0;
  unsigned long long utime = 0;
  unsigned long long stime = 0;
#if !defined(__APPLE__)
  const bool get_cpu_ok = get_cpu_total(&cpu_total);
  if (!get_cpu_ok) {
    log_server_error("get_cpu_total failed");
  }
#endif
  const uint16_t alive_workers_count = vk::singleton<WorkersControl>::get().get_all_alive();
  for (uint16_t i = 0; i < alive_workers_count; ++i) {
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
  }
  const auto general_workers_stat = vk::singleton<ServerStats>::get().collect_workers_stat(WorkerType::general_worker);
  server_stats.update_misc_stat_for_general_workers(MiscStatTimestamp{my_now, general_workers_stat.running_workers});
  const auto job_workers_stat = vk::singleton<ServerStats>::get().collect_workers_stat(WorkerType::job_worker);
  server_stats.update_misc_stat_for_job_workers(MiscStatTimestamp{my_now, job_workers_stat.running_workers});

  utime += dead_utime;
  stime += dead_stime;
  CpuStatTimestamp cpu_timestamp{my_now, utime, stime, cpu_total};
  server_stats.update(cpu_timestamp);

  vk::singleton<SharedData>::get().store_worker_stats({general_workers_stat.running_workers, general_workers_stat.waiting_workers,
                                                        general_workers_stat.ready_for_accept_workers, general_workers_stat.total_workers});
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

WorkerType run_master() {
  cpu_cnt = (int)sysconf(_SC_NPROCESSORS_ONLN);
  const auto &http_server_ports = vk::singleton<HttpServerContext>::get().http_ports();

  me->http_ports_count = http_server_ports.size();
  for (size_t i = 0; i < http_server_ports.size(); ++i) {
    me->http_ports[i] = http_server_ports[i];
  }

  me->own_http_fds = 0;

  epoll_sethandler(signal_fd, 0, signal_epoll_handler, nullptr);
  const int err = epoll_insert(signal_fd, EVT_READ);
  dl_assert (err >= 0, "epoll_insert failed");

  preallocate_msg_buffers();

  auto prev_cron_start_tp = get_steady_tp_ms_now();
  WarmUpContext::get().reset();
  while (true) {
    vkprintf(2, "run_master iteration: begin\n");

    my_now = dl_time();

    changed = 0;
    failed = 0;
    to_kill = 0;
    to_run = 0;
    to_exit = 0;
    job_workers_to_kill = job_workers_to_run = 0;

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

    if (to_kill != 0 || to_run != 0 || job_workers_to_kill != 0 || job_workers_to_run != 0) {
      vkprintf(1, "[to_kill = %d] [to_run = %d] [job_workers_to_kill = %d] [job_workers_to_run = %d]\n", to_kill, to_run, job_workers_to_kill, job_workers_to_run);
    }

    for (int i = 0; i < job_workers_to_kill; ++i) {
      kill_worker(WorkerType::job_worker);
    }
    for (int i = 0; i < job_workers_to_run; ++i) {
      if (run_worker(WorkerType::job_worker)) {
        tvkprintf(job_workers, 1, "launched new job worker with pid = %d\n", pid);
        return WorkerType::job_worker;
      }
    }

    while (to_kill-- > 0) {
      kill_worker(WorkerType::general_worker);
    }
    while (to_run-- > 0 && !failed) {
      if (run_worker(WorkerType::general_worker)) {
        return WorkerType::general_worker;
      }
    }
    kill_hanging_workers();

    me->running_http_workers_n = vk::singleton<WorkersControl>::get().get_running_count(WorkerType::general_worker);
    me->dying_http_workers_n = vk::singleton<WorkersControl>::get().get_dying_count(WorkerType::general_worker);;
    me->instance_cache_elements_cached = static_cast<uint32_t>(instance_cache_get_stats().elements_cached.load(std::memory_order_relaxed));

    if (state != master_state::off_in_graceful_shutdown) {
      if (changed && other->is_alive) {
        vkprintf(1, "wakeup other master [pid = %d]\n", (int)other->pid);
        kill(other->pid, SIGPOLL);
      }
    }

    shared_data_unlock(shared_data);

    if (to_exit) {
      vkprintf(1, "all workers killed. Exit\n");
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
