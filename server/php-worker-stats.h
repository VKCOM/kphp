// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <array>
#include <cinttypes>
#include <chrono>

#include "common/stats/provider.h"

#include "server/php-runner.h"

class PhpWorkerStats {
public:
  void add_stats(double script_time, double net_time, long script_queries,
                 long max_memory_used, long max_real_memory_used, script_error_t error) noexcept;

  void update_idle_time(double tot_idle_time, int uptime, double average_idle_time, double average_idle_quotient) noexcept;
  void recalc_worker_percentiles() noexcept;
  void recalc_master_percentiles() noexcept;

  void add_from(const PhpWorkerStats &from) noexcept;
  void copy_internal_from(const PhpWorkerStats &from) noexcept;

  std::string to_string(const std::string &pid_s = {}) const noexcept;
  void to_stats(stats_t *stats) const noexcept;

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

  std::array<std::chrono::steady_clock::time_point, PERCENTILE_SAMPLES> samples_tp_{};
  std::array<double, PERCENTILE_SAMPLES> working_time_samples_{};
  std::array<double, PERCENTILE_SAMPLES> net_time_samples_{};
  std::array<double, PERCENTILE_SAMPLES> script_time_samples_{};

  std::array<int64_t, PERCENTILE_SAMPLES> script_memory_used_samples_{};
  std::array<int64_t, PERCENTILE_SAMPLES> script_real_memory_used_samples_{};

  size_t circular_percentiles_counter_{0};

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

    uint32_t accumulated_stats_{0};
    std::array<uint32_t, static_cast<size_t>(script_error_t::errors_count)> errors_{{0}};

    std::array<double, PERCENTILES_COUNT> working_time_percentiles_{};
    std::array<double, PERCENTILES_COUNT> net_time_percentiles_{};
    std::array<double, PERCENTILES_COUNT> script_time_percentiles_{};

    std::array<int64_t, PERCENTILES_COUNT> script_memory_used_percentiles_{};
    std::array<int64_t, PERCENTILES_COUNT> script_real_memory_used_percentiles_{};
  } internal_;
};
