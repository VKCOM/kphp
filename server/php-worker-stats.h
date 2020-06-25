#pragma once

#include <array>
#include <cinttypes>

#include "common/percentile-estimator.h"
#include "common/stats/provider.h"

#include "server/php-runner.h"

class PhpWorkerStats {
public:
  void add_stats(double script_time, double net_time, long script_queries,
                 long max_memory_used, long max_real_memory_used, script_error_t error) noexcept;

  void update_idle_time(double tot_idle_time, int uptime, double average_idle_time, double average_idle_quotient) noexcept;

  void add_from(const PhpWorkerStats &from) noexcept;

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

  void reset_memory_usage() noexcept;

  struct PercentileEstimates {
    int64_t p50;
    int64_t p95;
    int64_t p99;
  };


private:
  void write_error_stat_to(stats_t *stats, const char *stat_name, script_error_t error) const noexcept;

  class DefaultPercentileEstimators {
  public:
    PercentileEstimates add_percentiles(PercentileEstimates estimates) {
      if (p50.estimate() || p95.estimate() || p99.estimate()) {
        p50.add_sample(estimates.p50);
        p95.add_sample(estimates.p95);
        p99.add_sample(estimates.p99);
      } else {
        p50 = vk::PercentileEstimator<50, int64_t>{estimates.p50};
        p95 = vk::PercentileEstimator<95, int64_t>{estimates.p95};
        p99 = vk::PercentileEstimator<99, int64_t>{estimates.p99};
      }
      return PercentileEstimates{p50.estimate(), p95.estimate(), p99.estimate()};
    }

    PercentileEstimates add_sample(int64_t sample) {
      return add_percentiles(PercentileEstimates{sample, sample, sample});
    }

    vk::PercentileEstimator<50, int64_t> p50;
    vk::PercentileEstimator<95, int64_t> p95;
    vk::PercentileEstimator<99, int64_t> p99;
  };

  DefaultPercentileEstimators total_request_time_ns_;
  DefaultPercentileEstimators net_time_ns_;
  DefaultPercentileEstimators script_time_ns_;

  DefaultPercentileEstimators script_memory_used_;
  DefaultPercentileEstimators script_real_memory_used_;

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

    PercentileEstimates working_time_ns_{0, 0, 0};
    PercentileEstimates net_time_ns_{0, 0, 0};
    PercentileEstimates script_time_ns_{0, 0, 0};

    PercentileEstimates script_memory_used_{0, 0, 0};
    PercentileEstimates script_real_memory_used_{0, 0, 0};
  } internal_;
};
