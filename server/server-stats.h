// Compiler for PHP (aka KPHP)
// Copyright (c) 2021 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <chrono>
#include <memory>
#include <random>

#include "common/mixin/not_copyable.h"
#include "common/smart_ptrs/singleton.h"
#include "common/stats/provider.h"

#include "server/php-runner.h"
#include "server/workers-control.h"

class ServerStats : vk::not_copyable {
public:
  void init() noexcept;

  void add_request_stats(double script_time_sec, double net_time_sec, int64_t script_queries, int64_t long_script_queries, int64_t memory_used,
                         int64_t real_memory_used, int64_t curl_total_allocated, script_error_t error) noexcept;
  void add_job_stats(double job_wait_time_sec, int64_t request_memory_used, int64_t request_real_memory_used, int64_t response_memory_used,
                     int64_t response_real_memory_used) noexcept;
  void add_job_common_memory_stats(int64_t common_request_memory_used, int64_t common_request_real_memory_used) noexcept;
  void update_this_worker_stats() noexcept;
  void update_active_connections(uint64_t active_connections, uint64_t max_connections) noexcept;

  void set_idle_worker_status() noexcept;
  void set_wait_net_worker_status() noexcept;
  void set_running_worker_status() noexcept;

  void after_fork(pid_t worker_pid, uint64_t active_connections, uint64_t max_connections,
                  uint16_t worker_process_id, WorkerType worker_type) noexcept;

  // these functions should be called only from the master process
  void aggregate_stats() noexcept;
  void write_stats_to(stats_t *stats) const noexcept;
  void write_stats_to(std::ostream &os, bool add_worker_pids) const noexcept;

  uint64_t get_worker_activity_counter(uint16_t worker_process_id) const noexcept;

  struct WorkersStat {
    uint16_t running_workers{0};
    uint16_t waiting_workers{0};
    uint16_t ready_for_accept_workers{0};
    uint16_t total_workers{0};
  };
  WorkersStat collect_workers_stat(WorkerType worker_type) const noexcept;
  std::tuple<uint64_t, uint64_t> collect_json_count_stat() const noexcept;

private:
  friend class vk::singleton<ServerStats>;

  ServerStats() = default;

  WorkerType worker_type_{WorkerType::general_worker};
  uint16_t worker_process_id_{0};
  std::chrono::steady_clock::time_point last_update_;

  std::mt19937 *gen_{nullptr};

  struct AggregatedStats;
  AggregatedStats *aggregated_stats_{nullptr};

  struct SharedStats;
  SharedStats *shared_stats_{nullptr};
};
