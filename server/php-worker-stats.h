// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <array>
#include <cinttypes>
#include <chrono>

#include "common/stats/provider.h"

#include "server/php-runner.h"
#include "server/workers-control.h"

// TODO Split this class into different entities:
//    KphpProcessStats - stats about general staff: memory, cpu, idle, ...
//    PhpWorkerStats inherits KphpProcessStats - stats about request processing: script time, memory usage, ...
//    KphpMasterStats inherits KphpProcessStats - stats about master process (may be exactly the same as KphpProcessStats)
//    KphpStats - can aggregate, calc percentile, write stats to statsd, string and so on ...
// Also, take a look into php-master.cpp, there are a bunch of other stats.
class PhpWorkerStats {
public:
  void add_stats(double script_time, double net_time, int64_t script_queries,
                 int64_t max_memory_used, int64_t max_real_memory_used, int64_t heap_memory_used, script_error_t error) noexcept;

  void add_job_wait_time_stats(double job_wait_time) noexcept;

  void update_idle_time(double tot_idle_time, int uptime, double average_idle_time, double average_idle_quotient) noexcept;
  void update_mem_info() noexcept;
  void recalc_worker_percentiles() noexcept;

  void add_worker_stats_from(const PhpWorkerStats &from, WorkerType worker_type) noexcept;
  void copy_internal_from(const PhpWorkerStats &from) noexcept;

  std::string to_string(const std::string &pid_s = {}) const noexcept;
  void to_stats(stats_t *stats) noexcept;

  int write_into(char *buffer, int buffer_len) const noexcept;
  int read_from(const char *buffer) noexcept;

  static PhpWorkerStats &get_local() noexcept {
    static PhpWorkerStats this_;
    return this_;
  }

  long total_queries() const noexcept { return internal_.tot_queries_; }
  long total_script_queries() const noexcept { return internal_.tot_script_queries_; }

  void reset_memory_and_percentiles_stats() noexcept;

private:
  void write_error_stat_to(stats_t *stats, const char *stat_name, script_error_t error) const noexcept;

  // 0 - 50 percentile, 1 - 95 percentile, 2 - 99 percentile
  static constexpr size_t PERCENTILES_COUNT{3};
  // the number of samples in the selection
  static constexpr size_t PERCENTILE_SAMPLES{600};
  static_assert(PERCENTILE_SAMPLES % PERCENTILES_COUNT == 0, "bad PERCENTILE_SAMPLES value");

  size_t job_wait_time_percentiles_counter_{0};
  std::array<double, PERCENTILE_SAMPLES> job_wait_time_samples_{};
  std::array<std::chrono::steady_clock::time_point, PERCENTILE_SAMPLES> job_samples_tp_{};

  size_t request_circular_percentiles_counter_{0};
  std::array<std::chrono::steady_clock::time_point, PERCENTILE_SAMPLES> samples_tp_{};
  std::array<double, PERCENTILE_SAMPLES> working_time_samples_{};
  std::array<double, PERCENTILE_SAMPLES> net_time_samples_{};
  std::array<double, PERCENTILE_SAMPLES> script_time_samples_{};

  std::array<int64_t, PERCENTILE_SAMPLES> script_memory_used_samples_{};
  std::array<int64_t, PERCENTILE_SAMPLES> script_real_memory_used_samples_{};

  size_t worker_circular_percentiles_counter_{0};
  std::array<int64_t, PERCENTILE_SAMPLES> workers_script_heap_usage_bytes_{};
  std::array<uint32_t, PERCENTILE_SAMPLES> workers_malloc_non_mapped_total_used_bytes_{};
  std::array<uint32_t, PERCENTILE_SAMPLES> workers_malloc_non_mapped_allocated_bytes_{};
  std::array<uint32_t, PERCENTILE_SAMPLES> workers_malloc_non_mapped_free_bytes_{};
  std::array<uint32_t, PERCENTILE_SAMPLES> workers_malloc_mmaped_bytes_{};

  std::array<int64_t, PERCENTILE_SAMPLES> workers_rss_usage_{};
  std::array<int64_t, PERCENTILE_SAMPLES> workers_vms_usage_{};
  std::array<int64_t, PERCENTILE_SAMPLES> workers_shm_usage_{};

  int64_t rss_kb_sum_{0};
  int64_t vms_kb_sum_{0};

  uint32_t rss_kb_max_{0};
  uint32_t vms_kb_max_{0};

  int64_t rss_no_shm_kb_sum_{0};

  struct {
    int64_t tot_queries_{0};
    int64_t tot_script_queries_{0};

    double net_time_{0};
    double script_time_{0};

    double tot_idle_time_{0};
    double tot_idle_percent_{0};
    double a_idle_percent_{0};

    int64_t script_max_memory_used_{0};
    int64_t script_max_real_memory_used_{0};
    int64_t script_heap_memory_usage_{0};

    mem_info_t mem_info_{};
    struct {
      uint32_t total_non_mmaped_allocated_bytes_{0};
      uint32_t total_non_mmaped_free_bytes_{0};
      uint32_t total_mmaped_bytes_{0};
    } malloc_stats_;

    uint32_t accumulated_stats_{0};
    std::array<uint32_t, static_cast<size_t>(script_error_t::errors_count)> errors_{{0}};

    std::array<double, PERCENTILES_COUNT> working_time_percentiles_{};
    std::array<double, PERCENTILES_COUNT> net_time_percentiles_{};
    std::array<double, PERCENTILES_COUNT> script_time_percentiles_{};

    std::array<double, PERCENTILES_COUNT> job_wait_time_percentiles_{};

    std::array<int64_t, PERCENTILES_COUNT> script_memory_used_percentiles_{};
    std::array<int64_t, PERCENTILES_COUNT> script_real_memory_used_percentiles_{};
  } internal_;
};
