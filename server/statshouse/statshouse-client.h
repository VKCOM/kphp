// Compiler for PHP (aka KPHP)
// Copyright (c) 2023 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include "third-party/statshouse.h"

#include <cassert>

#include "common/dl-utils-lite.h"
#include "common/mixin/not_copyable.h"
#include "runtime/memory_resource/memory_resource.h"
#include "server/job-workers/job-stats.h"
#include "server/workers-control.h"
#include "server/workers-stats.h"

class StatsHouseClient : vk::not_copyable {
public:
  static void init(std::string ip, int port) {
    static StatsHouseClient client{ip, port};
    inner = &client;
  }

  static bool has() {
    return inner != nullptr;
  }

  static StatsHouseClient &get() {
    assert(inner);
    return *inner;
  }

  void send_request_stats(WorkerType raw_worker_type, uint64_t script_time_ns, uint64_t net_time_ns, uint64_t memory_used, uint64_t real_memory_used,
                         uint64_t script_queries, uint64_t long_script_queries);

  void send_job_stats(uint64_t job_wait_ns, uint64_t request_memory_used, uint64_t request_real_memory_used, uint64_t response_memory_used,
                     uint64_t response_real_memory_used);

  void send_job_common_memory_stats(uint64_t job_common_request_memory_used, uint64_t job_common_request_real_memory_used);

  void send_worker_memory_stats(WorkerType raw_worker_type, const mem_info_t &mem_stats);

  /**
   * Must be called from master process only
   */
  void send_common_master_stats(const workers_stats_t &workers_stats, const memory_resource::MemoryStats &memory_stats, double cpu_s_usage, double cpu_u_usage,
                               long long int instance_cache_memory_swaps_ok, long long int instance_cache_memory_swaps_fail);

private:
  explicit StatsHouseClient(const std::string &ip, int port);

  void add_job_workers_shared_memory_stats(const char *cluster_name, const job_workers::JobStats &job_stats);

  size_t add_job_workers_shared_messages_stats(const char *cluster_name, const job_workers::JobStats::MemoryBufferStats &memory_buffers_stats,
                                               size_t buffer_size);

  size_t add_job_workers_shared_memory_buffers_stats(const char *cluster_name, const job_workers::JobStats::MemoryBufferStats &memory_buffers_stats,
                                                     const char *size_tag, size_t buffer_size);

  static StatsHouseClient *inner;
  statshouse::TransportUDP transport;
};
