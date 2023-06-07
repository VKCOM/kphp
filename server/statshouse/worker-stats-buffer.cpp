// Compiler for PHP (aka KPHP)
// Copyright (c) 2022 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "worker-stats-buffer.h"
#include "common/resolver.h"
#include "common/tl/constants/statshouse.h"
#include "runtime/critical_section.h"
#include "server/server-config.h"
#include "statshouse-client.h"

namespace statshouse {

constexpr size_t buffer_size = 1 << 20;
static char buffer[buffer_size];

bool StatsBuffer::is_need_to_flush() {
  return size >= data.size();
}

bool StatsBuffer::empty() const {
  return size == 0;
}

void StatsBuffer::add_stat(double value) {
  data[size++] = value;
}

std::vector<double> StatsBuffer::get_data_and_reset_buffer() {
  std::vector<double> result(data.begin(), data.begin() + size);
  size = 0;
  return result;
}

WorkerStatsBuffer::WorkerStatsBuffer() : last_send_time(std::chrono::steady_clock::now()) {

}

void WorkerStatsBuffer::add_query_stat(GenericQueryStatKey key, WorkerType worker_type, double value) {
  if (!enabled) {
    return;
  }
  auto &worker_type_buffer = generic_query_stats[static_cast<size_t>(worker_type)];
  auto &stats_buffer = worker_type_buffer[static_cast<size_t>(key)];

  if (stats_buffer.is_need_to_flush()) {
    flush();
  }
  stats_buffer.add_stat(value);
}

void WorkerStatsBuffer::add_query_stat(QueryStatKey key, double value) {
  if (!enabled) {
    return;
  }
  auto &stats_buffer = query_stats[static_cast<size_t>(key)];

  if (stats_buffer.is_need_to_flush()) {
    flush();
  }
  stats_buffer.add_stat(value);
}

void WorkerStatsBuffer::make_generic_metric(std::vector<StatsHouseMetric> &metrics, const char *name, GenericQueryStatKey stat_key, size_t worker_type,
                                            const std::vector<tag> &tags) {
  auto &stats_buffer = generic_query_stats[worker_type][static_cast<size_t>(stat_key)];
  if (!stats_buffer.empty()) {
    metrics.push_back(make_statshouse_value_metrics(name, stats_buffer.get_data_and_reset_buffer(), tags));
  }
}

void WorkerStatsBuffer::make_metric(std::vector<StatsHouseMetric> &metrics, const char *name, QueryStatKey stat_key, const std::vector<tag> &tags) {
  auto &stats_buffer = query_stats[static_cast<size_t>(stat_key)];
  if (!stats_buffer.empty()) {
    metrics.push_back(make_statshouse_value_metrics(name, stats_buffer.get_data_and_reset_buffer(), tags));
  }
}

void WorkerStatsBuffer::flush() {
  dl::CriticalSectionGuard critical_section;
  std::vector<StatsHouseMetric> metrics;
  const char *cluster_name = vk::singleton<ServerConfig>::get().get_cluster_name();

  for (size_t i = 0; i < static_cast<size_t>(WorkerType::types_count); ++i) {
    std::vector<tag> tags;
    tags.emplace_back("cluster", cluster_name);
    tags.emplace_back("worker_type", i == static_cast<size_t>(WorkerType::general_worker) ? "general" : "job");

    make_generic_metric(metrics, "kphp_requests_outgoing_queries", GenericQueryStatKey::outgoing_queries, i, tags);
    make_generic_metric(metrics, "kphp_requests_outgoing_long_queries", GenericQueryStatKey::outgoing_long_queries, i, tags);
    make_generic_metric(metrics, "kphp_requests_script_time", GenericQueryStatKey::script_time, i, tags);
    make_generic_metric(metrics, "kphp_requests_net_time", GenericQueryStatKey::net_time, i, tags);

    make_generic_metric(metrics, "kphp_memory_script_usage", GenericQueryStatKey::memory_used, i, tags);
    make_generic_metric(metrics, "kphp_memory_script_real_usage", GenericQueryStatKey::real_memory_used, i, tags);
  }

  std::vector<tag> tags;
  tags.emplace_back("cluster_name", cluster_name);

  make_metric(metrics, "kphp_jobs_queue_time", QueryStatKey::job_wait_time, tags);
  make_metric(metrics, "kphp_memory_job_request_usage", QueryStatKey::job_request_memory_usage, tags);
  make_metric(metrics, "kphp_memory_job_request_real_usage", QueryStatKey::job_request_real_memory_usage, tags);
  make_metric(metrics, "kphp_memory_job_response_usage", QueryStatKey::job_response_memory_usage, tags);
  make_metric(metrics, "kphp_memory_job_response_real_usage", QueryStatKey::job_response_real_memory_usage, tags);

  make_metric(metrics, "kphp_memory_job_common_request_usage", QueryStatKey::job_common_request_memory_usage, tags);
  make_metric(metrics, "kphp_memory_job_common_request_real_usage", QueryStatKey::job_common_request_real_memory_usage, tags);

  if (metrics.empty()) {
    last_send_time = std::chrono::steady_clock::now();
    return;
  }

  int offset = vk::tl::store_to_buffer(buffer, buffer_size, TL_STATSHOUSE_ADD_METRICS_BATCH);
  offset = offset + vk::tl::store_to_buffer(buffer + offset, buffer_size - offset, static_cast<int>(vk::tl::statshouse::add_metrics_batch_fields_mask::ALL));
  int len = vk::tl::store_to_buffer(buffer + offset, buffer_size - offset, metrics);
  vk::singleton<StatsHouseClient>::get().send_metrics(buffer, offset + len);
  last_send_time = std::chrono::steady_clock::now();
}

void WorkerStatsBuffer::flush_if_needed() {
  const auto now_tp = std::chrono::steady_clock::now();
  if (enabled && (now_tp - last_send_time >= std::chrono::seconds{1})) {
    vk::singleton<statshouse::WorkerStatsBuffer>::get().flush();
  }
}
void WorkerStatsBuffer::enable() {
  enabled = true;
}

} // namespace statshouse
