// Compiler for PHP (aka KPHP)
// Copyright (c) 2023 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cassert>

#include "common/dl-utils-lite.h"
#include "common/mixin/not_copyable.h"
#include "runtime/memory_resource/memory_resource.h"
#include "server/job-workers/job-stats.h"
#include "server/php-runner.h"
#include "server/statshouse/statshouse-client.h"
#include "server/workers-control.h"
#include "server/workers-stats.h"

class StatsHouseMetrics : vk::not_copyable {
public:
  static void init_client(const std::string &ip, int port) {
    storage_impl(ip, port);
  }

  static StatsHouseMetrics &get() {
    return storage_impl({}, {});
  }

  void init_common_tags(std::string_view cluster, std::string_view host) {
    client.init_common_tags(cluster, host);
  }

  void on_worker_cron() {
    generic_cron_check_if_tag_host_needed();
  }

  void on_master_cron() {
    generic_cron_check_if_tag_host_needed();
  }

  /**
   * This toggle turns on host tag for all statshouse metrics in all workers and master processes during the next 30 sec.
   * See generic_cron_check_if_tag_host_needed() for details.
   */
  void turn_on_host_tag_toggle() {
    need_write_enable_tag_host = true;
  }

  void add_request_stats(uint64_t script_time_ns, uint64_t net_time_ns, script_error_t error, uint64_t memory_used, uint64_t real_memory_used,
                         uint64_t script_queries, uint64_t long_script_queries);

  void add_job_stats(uint64_t job_wait_ns, uint64_t request_memory_used, uint64_t request_real_memory_used, uint64_t response_memory_used,
                     uint64_t response_real_memory_used);

  void add_job_common_memory_stats(uint64_t job_common_request_memory_used, uint64_t job_common_request_real_memory_used);

  void add_worker_memory_stats(const mem_info_t &mem_stats);

  /**
   * Must be called from master process only
   */
  void add_common_master_stats(const workers_stats_t &workers_stats, const memory_resource::MemoryStats &memory_stats, double cpu_s_usage, double cpu_u_usage,
                               long long int instance_cache_memory_swaps_ok, long long int instance_cache_memory_swaps_fail);

private:
  StatsHouseClient client;
  bool need_write_enable_tag_host = false;

  StatsHouseMetrics() = default;
  explicit StatsHouseMetrics(const std::string &ip, int port);

  // returns safe to use dummy instance if wasn't initialized
  static StatsHouseMetrics &storage_impl(const std::string &ip, int port) {
    static StatsHouseMetrics client{ip, port};
    return client;
  }

  void generic_cron_check_if_tag_host_needed();

  void add_job_workers_shared_memory_stats(const job_workers::JobStats &job_stats);

  size_t add_job_workers_shared_messages_stats(const job_workers::JobStats::MemoryBufferStats &memory_buffers_stats,
                                               size_t buffer_size);

  size_t add_job_workers_shared_memory_buffers_stats(const job_workers::JobStats::MemoryBufferStats &memory_buffers_stats,
                                                     const char *size_tag, size_t buffer_size);
};
