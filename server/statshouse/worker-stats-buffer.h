// Compiler for PHP (aka KPHP)
// Copyright (c) 2022 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#pragma once

#include <array>
#include <chrono>
#include <ostream>

#include "add-metrics-batch.h"
#include "common/mixin/not_copyable.h"
#include "common/tl/methods/string.h"
#include "server/workers-control.h"

namespace statshouse {

enum class GenericQueryStatKey {
  outgoing_queries,
  outgoing_long_queries,
  script_time,
  net_time,

  memory_used,
  real_memory_used,
  total_allocated_by_curl,

  types_count
};

enum class QueryStatKey {
  job_wait_time,
  job_request_memory_usage,
  job_request_real_memory_usage,
  job_response_memory_usage,
  job_response_real_memory_usage,

  job_common_request_memory_usage,
  job_common_request_real_memory_usage,

  types_count
};

class StatsBuffer : vk::not_copyable {
public:
  bool is_need_to_flush();
  bool empty() const;
  void add_stat(double value);
  std::vector<double> get_data_and_reset_buffer();

private:
  std::array<double, 50> data;
  size_t size{0};
};

class WorkerStatsBuffer : vk::not_copyable {
public:
  WorkerStatsBuffer();
  void add_query_stat(GenericQueryStatKey key, WorkerType worker_type, double value);
  void add_query_stat(QueryStatKey key, double value);
  void flush_if_needed();
  void enable();
  using tag = std::pair<std::string, std::string>;

private:
  void flush();
  void make_generic_metric(std::vector<StatsHouseMetric> &metrics, const char *name, GenericQueryStatKey stat_key, size_t worker_type, long long timestamp,
                           const std::vector<tag> &tags);
  void make_metric(std::vector<StatsHouseMetric> &metrics, const char *name, QueryStatKey stat_key, long long timestamp, const std::vector<tag> &tags);

  std::array<std::array<StatsBuffer, static_cast<size_t>(GenericQueryStatKey::types_count)>, static_cast<size_t>(WorkerType::types_count)> generic_query_stats;
  std::array<StatsBuffer, static_cast<size_t>(QueryStatKey::types_count)> query_stats;
  std::chrono::steady_clock::time_point last_send_time;
  bool enabled = false;
};

} // namespace statshouse
