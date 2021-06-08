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
  void add_request_stats(double script_time_sec, double net_time_sec, int64_t script_queries,
                         int64_t memory_used, int64_t real_memory_used, int64_t curl_total_allocated,
                         script_error_t error) noexcept;
  void add_job_stats(double job_wait_time_sec, int64_t memory_used, int64_t real_memory_used) noexcept;
  void update_this_worker_stats() noexcept;

  void after_fork(pid_t worker_pid, size_t worker_process_id, WorkerType worker_type) noexcept;

  // these functions should be called only from the master process
  void aggregate_stats() noexcept;
  void write_stats_to(stats_t *stats) const noexcept;
  void write_stats_to(std::ostream &os) const noexcept;

  ~ServerStats() noexcept;

private:
  friend class vk::singleton<ServerStats>;

  ServerStats() noexcept;

  WorkerType worker_type_{WorkerType::general_worker};
  uint16_t worker_process_id_{0};
  std::mt19937 gen_;
  std::chrono::steady_clock::time_point last_update_;

  struct SharedStats;
  SharedStats *shared_stats_{nullptr};

  struct AggregatedStats;
  std::unique_ptr<AggregatedStats> aggregated_stats_;
};
