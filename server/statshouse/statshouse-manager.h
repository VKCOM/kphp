// Compiler for PHP (aka KPHP)
// Copyright (c) 2023 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <cassert>
#include <functional>

#include "common/dl-utils-lite.h"
#include "common/mixin/not_copyable.h"
#include "common/binlog/binlog-stats.h"
#include "runtime-common/core/memory-resource/memory_resource.h"
#include "runtime/runtime-builtin-stats.h"
#include "server/job-workers/job-stats.h"
#include "server/php-queries.h"
#include "server/statshouse/statshouse-client.h"
#include "server/workers-control.h"
#include "server/workers-stats.h"

using normalization_function = std::function<string(const string &)>;

enum class script_error_t : uint8_t;

struct ConfdataStats;

class StatsHouseManager : vk::not_copyable {
public:
  static void init(const std::string &ip, int port) {
    storage_impl(ip, port);
  }

  static StatsHouseManager &get() {
    return storage_impl({}, {});
  }

  void set_common_tags();

  // Runs in master and workers cron with 1 sec period
  void generic_cron() {
    generic_cron_check_if_tag_host_needed();
    set_common_tags();
  }

  /**
   * This toggle turns on host tag for all statshouse metrics in all workers and master processes during the next 30 sec.
   * See generic_cron_check_if_tag_host_needed() for details.
   */
  void turn_on_host_tag_toggle() {
    need_write_enable_tag_host = true;
  }

  void set_normalization_function(normalization_function &&_function) {
    this->instance_cache_key_normalization_function = std::move(_function);
  }

  bool is_extended_instance_cache_stats_enabled() {
    return this->instance_cache_key_normalization_function != nullptr;
  }

  void add_request_stats(uint64_t script_time_ns, uint64_t net_time_ns, uint64_t script_max_running_interval_ns, script_error_t error,
                         const memory_resource::MemoryStats &script_memory_stats, const runtime_builtins_stats::request_stats_t &builtin_stats,
                         uint64_t script_queries, uint64_t long_script_queries,
                         uint64_t script_user_time_ns, uint64_t script_system_time_ns,
                         uint64_t script_init_time, uint64_t http_connection_process_time,
                         uint64_t voluntary_context_switches, uint64_t involuntary_context_switches);

  void add_job_stats(uint64_t job_wait_ns, uint64_t request_memory_used, uint64_t request_real_memory_used, uint64_t response_memory_used,
                     uint64_t response_real_memory_used);

  void add_job_common_memory_stats(uint64_t job_common_request_memory_used, uint64_t job_common_request_real_memory_used);

  void add_worker_memory_stats(const mem_info_t &mem_stats);

  /**
   * Must be called from master process only
   */
  void add_common_master_stats(const workers_stats_t &workers_stats,
                               const memory_resource::MemoryStats &instance_cache_memory_stats,
                               double cpu_s_usage, double cpu_u_usage,
                               long long int instance_cache_memory_swaps_ok, long long int instance_cache_memory_swaps_fail);

  /**
   * Must be called from master process only
   */
  void add_init_master_stats(uint64_t total_init_ns, uint64_t confdata_init_ns);

  /**
   * before calling the method, be sure to is_extended_instance_cache_stats_enabled() is true
   */
  void add_extended_instance_cache_stats(std::string_view type, std::string_view status, const string &key, uint64_t size = 0);

  void add_confdata_master_stats(const ConfdataStats &confdata_stats);

  void add_confdata_binlog_reader_stats(const binlog_reader_stats& confdata_stats) noexcept;

  void add_slow_net_event_stats(const slow_net_event_stats::stats_t &stats) noexcept;

private:
  StatsHouseClient client;
  bool need_write_enable_tag_host = false;
  normalization_function instance_cache_key_normalization_function = nullptr;

  StatsHouseManager() = default;
  explicit StatsHouseManager(const std::string &ip, int port);

  // returns safe to use dummy instance if wasn't initialized
  static StatsHouseManager &storage_impl(const std::string &ip, int port) {
    static StatsHouseManager client{ip, port};
    return client;
  }

  void generic_cron_check_if_tag_host_needed();

  void add_job_workers_shared_memory_stats(const job_workers::JobStats &job_stats);

  size_t add_job_workers_shared_messages_stats(const job_workers::JobStats::MemoryBufferStats &memory_buffers_stats,
                                               size_t buffer_size);

  size_t add_job_workers_shared_memory_buffers_stats(const job_workers::JobStats::MemoryBufferStats &memory_buffers_stats,
                                                     const char *size_tag, size_t buffer_size);
};
