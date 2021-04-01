// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include <algorithm>
#include <cassert>
#include <cstring>

#include "common/macos-ports.h"

#include "server/php-worker-stats.h"

namespace {

template<size_t P1, size_t P2, size_t P3, class T, size_t N>
std::array<T, 3> calc_percentiles(std::array<T, N> &samples) noexcept {
  const auto last = std::remove(samples.begin(), samples.end(), 0);
  const auto size = last - samples.begin();
  std::nth_element(samples.begin(), samples.begin() + P1 * size / 100, last);
  std::nth_element(samples.begin(), samples.begin() + P2 * size / 100, last);
  std::nth_element(samples.begin(), samples.begin() + P3 * size / 100, last);
  return std::array<T, 3>{samples[P1 * size / 100], samples[P2 * size / 100], samples[P3 * size / 100]};
}

template<size_t P1, size_t P2, size_t P3, class T, size_t N>
void write_percentile(stats_t *stats, const char *prefix, std::array<T, N> &samples) noexcept {
  const auto values = calc_percentiles<P1, P2, P3>(samples);
  add_gauge_stat(stats, values[0], prefix, ".percentile_50");
  add_gauge_stat(stats, values[1], prefix, ".percentile_95");
  add_gauge_stat(stats, values[2], prefix, ".percentile_99");
}

template<class T, size_t N>
std::array<T, 3> calc_timed_50_95_99_percentiles(std::array<T, N> samples,
                                                 const std::array<std::chrono::steady_clock::time_point, N> &samples_tp,
                                                 std::chrono::steady_clock::time_point now_tp) {
  for (size_t i = 0; i < samples_tp.size(); ++i) {
    if (now_tp - samples_tp[i] > std::chrono::minutes{1}) {
      samples[i] = 0;
    }
  }
  return calc_percentiles<50, 95, 99>(samples);
}

template<class T, size_t M, size_t N>
void append_samples(const std::array<T, M> &from, std::array<T, N> &to, size_t offset) {
  assert(offset + M <= N);
  std::copy_n(from.begin(), M, to.begin() + offset);
}

int64_t get_shm(mem_info_t mem) noexcept {
  return mem.rss_file + mem.rss_shmem;
}

int64_t get_rss_without_shm(mem_info_t mem) noexcept {
  return int64_t{mem.rss} - get_shm(mem);
}

} // namespace

void PhpWorkerStats::add_stats(double script_time, double net_time, int64_t script_queries,
                               int64_t max_memory_used, int64_t max_real_memory_used, int64_t heap_memory_used, script_error_t error) noexcept {
  internal_.tot_queries_++;
  internal_.net_time_ += net_time;
  internal_.script_time_ += script_time;
  internal_.tot_script_queries_ += script_queries;
  internal_.script_max_memory_used_ = std::max(internal_.script_max_memory_used_, int64_t{max_memory_used});
  internal_.script_max_real_memory_used_ = std::max(internal_.script_max_real_memory_used_, int64_t{max_real_memory_used});
  internal_.script_heap_memory_usage_ = heap_memory_used;
  ++internal_.errors_[static_cast<size_t>(error)];

  const size_t sample = request_circular_percentiles_counter_++;
  request_circular_percentiles_counter_ %= PERCENTILE_SAMPLES;
  samples_tp_[sample] = std::chrono::steady_clock::now();
  working_time_samples_[sample] = script_time + net_time;
  net_time_samples_[sample] = net_time;
  script_time_samples_[sample] = script_time;
  script_memory_used_samples_[sample] = max_memory_used;
  script_real_memory_used_samples_[sample] = max_real_memory_used;
}

void PhpWorkerStats::update_idle_time(double tot_idle_time, int uptime, double average_idle_time, double average_idle_quotient) noexcept {
  internal_.tot_idle_time_ = tot_idle_time;
  internal_.tot_idle_percent_ = uptime > 0 ? tot_idle_time / uptime * 100 : 0;
  internal_.a_idle_percent_ = average_idle_quotient > 0 ? average_idle_time / average_idle_quotient * 100 : 0;
}

void PhpWorkerStats::update_mem_info() noexcept {
  internal_.mem_info_ = get_self_mem_stats();

  rss_kb_sum_ = internal_.mem_info_.rss;
  vms_kb_sum_ = internal_.mem_info_.vm;

  rss_kb_max_ = internal_.mem_info_.rss_peak;
  vms_kb_max_ = internal_.mem_info_.vm_peak;

  rss_no_shm_kb_sum_ = get_rss_without_shm(internal_.mem_info_);

  const auto malloc_stats = mallinfo();
  internal_.malloc_stats_.total_mmaped_bytes_ = malloc_stats.hblkhd;
  internal_.malloc_stats_.total_non_mmaped_allocated_bytes_ = malloc_stats.uordblks;
  internal_.malloc_stats_.total_non_mmaped_free_bytes_ = malloc_stats.fordblks;
}

void PhpWorkerStats::recalc_worker_percentiles() noexcept {
  const auto now_tp = std::chrono::steady_clock::now();
  internal_.working_time_percentiles_ = calc_timed_50_95_99_percentiles(working_time_samples_, samples_tp_, now_tp);
  internal_.net_time_percentiles_ = calc_timed_50_95_99_percentiles(net_time_samples_, samples_tp_, now_tp);
  internal_.script_time_percentiles_ = calc_timed_50_95_99_percentiles(script_time_samples_, samples_tp_, now_tp);

  internal_.script_memory_used_percentiles_ = calc_timed_50_95_99_percentiles(script_memory_used_samples_, samples_tp_, now_tp);
  internal_.script_real_memory_used_percentiles_ = calc_timed_50_95_99_percentiles(script_real_memory_used_samples_, samples_tp_, now_tp);
}

void PhpWorkerStats::add_worker_stats_from(const PhpWorkerStats &from) noexcept {
  internal_.tot_queries_ += from.internal_.tot_queries_;
  internal_.net_time_ += from.internal_.net_time_;
  internal_.script_time_ += from.internal_.script_time_;
  internal_.tot_script_queries_ += from.internal_.tot_script_queries_;
  internal_.tot_idle_time_ += from.internal_.tot_idle_time_;
  internal_.tot_idle_percent_ += from.internal_.tot_idle_percent_;
  internal_.a_idle_percent_ += from.internal_.a_idle_percent_;
  internal_.script_max_memory_used_ = std::max(internal_.script_max_memory_used_, from.internal_.script_max_memory_used_);
  internal_.script_max_real_memory_used_ = std::max(internal_.script_max_real_memory_used_, from.internal_.script_max_real_memory_used_);

  internal_.accumulated_stats_++;
  for (size_t i = 0; i < internal_.errors_.size(); ++i) {
    internal_.errors_[i] += from.internal_.errors_[i];
  }

  const size_t request_offset = request_circular_percentiles_counter_;
  request_circular_percentiles_counter_ = (request_circular_percentiles_counter_ + PERCENTILES_COUNT) % PERCENTILE_SAMPLES;

  append_samples(from.internal_.working_time_percentiles_, working_time_samples_, request_offset);
  append_samples(from.internal_.net_time_percentiles_, net_time_samples_, request_offset);
  append_samples(from.internal_.script_time_percentiles_, script_time_samples_, request_offset);

  append_samples(from.internal_.script_memory_used_percentiles_, script_memory_used_samples_, request_offset);
  append_samples(from.internal_.script_real_memory_used_percentiles_, script_real_memory_used_samples_, request_offset);

  const size_t worker_offset = worker_circular_percentiles_counter_++;
  worker_circular_percentiles_counter_ %= PERCENTILE_SAMPLES;
  workers_script_heap_usage_bytes_[worker_offset] = from.internal_.script_heap_memory_usage_;
  workers_malloc_non_mapped_total_used_bytes_[worker_offset] = from.internal_.malloc_stats_.total_non_mmaped_allocated_bytes_ + from.internal_.malloc_stats_.total_non_mmaped_free_bytes_;
  workers_malloc_non_mapped_allocated_bytes_[worker_offset] = from.internal_.malloc_stats_.total_non_mmaped_allocated_bytes_;
  workers_malloc_non_mapped_free_bytes_[worker_offset] = from.internal_.malloc_stats_.total_non_mmaped_free_bytes_;
  workers_malloc_mmaped_bytes_[worker_offset] = from.internal_.malloc_stats_.total_mmaped_bytes_;

  workers_rss_usage_[worker_offset] = int64_t{from.internal_.mem_info_.rss} * 1024;
  workers_vms_usage_[worker_offset] = int64_t{from.internal_.mem_info_.vm} * 1024;
  workers_shm_usage_[worker_offset] = get_shm(from.internal_.mem_info_) * 1024;

  rss_kb_sum_ += from.internal_.mem_info_.rss;
  vms_kb_sum_ += from.internal_.mem_info_.vm;

  rss_kb_max_ = std::max(rss_kb_max_, from.internal_.mem_info_.rss_peak);
  vms_kb_max_ = std::max(vms_kb_max_, from.internal_.mem_info_.vm_peak);

  rss_no_shm_kb_sum_ += get_rss_without_shm(from.internal_.mem_info_);
}

void PhpWorkerStats::copy_internal_from(const PhpWorkerStats &from) noexcept {
  internal_ = from.internal_;
}

std::string PhpWorkerStats::to_string(const std::string &pid_s) const noexcept {
  char buf[1000] = {0};
  std::string res;

  const int cnt = std::max(internal_.accumulated_stats_, 1U);

  if (pid_s.empty()) {
    sprintf(buf, "VM\t%" PRIi64 "Kb\n", vms_kb_sum_);
    res += buf;
    sprintf(buf, "VM_max\t%" PRIu32 "Kb\n", vms_kb_max_);
    res += buf;
    sprintf(buf, "RSS\t%" PRIi64 "Kb\n", rss_kb_sum_);
    res += buf;
    sprintf(buf, "RSS_max\t%" PRIu32 "Kb\n", rss_kb_max_);
    res += buf;
  } else {
    sprintf(buf, "VM%s\t%" PRIu32 "Kb\n", pid_s.c_str(), internal_.mem_info_.vm);
    res += buf;
    sprintf(buf, "VM_max%s\t%" PRIu32 "Kb\n", pid_s.c_str(), internal_.mem_info_.vm_peak);
    res += buf;
    sprintf(buf, "RSS%s\t%" PRIu32 "Kb\n", pid_s.c_str(), internal_.mem_info_.rss);
    res += buf;
    sprintf(buf, "RSS_max%s\t%" PRIu32 "Kb\n", pid_s.c_str(), internal_.mem_info_.rss_peak);
    res += buf;
  }

  sprintf(buf, "tot_queries%s\t%" PRIi64 "\n", pid_s.c_str(), internal_.tot_queries_);
  res += buf;
  sprintf(buf, "worked_time%s\t%.3lf\n", pid_s.c_str(), internal_.script_time_ + internal_.net_time_);
  res += buf;
  sprintf(buf, "net_time%s\t%.3lf\n", pid_s.c_str(), internal_.net_time_);
  res += buf;
  sprintf(buf, "script_time%s\t%.3lf\n", pid_s.c_str(), internal_.script_time_);
  res += buf;
  sprintf(buf, "tot_script_queries%s\t%" PRIi64 "\n", pid_s.c_str(), internal_.tot_script_queries_);
  res += buf;
  sprintf(buf, "tot_idle_time%s\t%.3lf\n", pid_s.c_str(), internal_.tot_idle_time_);
  res += buf;
  sprintf(buf, "tot_idle_percent%s\t%.3lf%%\n", pid_s.c_str(), internal_.tot_idle_percent_ / cnt);
  res += buf;
  sprintf(buf, "recent_idle_percent%s\t%.3lf%%\n", pid_s.c_str(), internal_.a_idle_percent_ / cnt);
  res += buf;

  return res;
}

void PhpWorkerStats::to_stats(stats_t *stats) noexcept {
  add_gauge_stat_long(stats, "requests.total_incoming_queries", internal_.tot_queries_);
  add_gauge_stat_long(stats, "requests.total_outgoing_queries", internal_.tot_script_queries_);
  add_gauge_stat_double(stats, "requests.script_time.total", internal_.script_time_);
  add_gauge_stat_double(stats, "requests.net_time.total", internal_.net_time_);

  add_gauge_stat_long(stats, "memory.script_usage.max", internal_.script_max_memory_used_);
  add_gauge_stat_long(stats, "memory.script_real_usage.max", internal_.script_max_real_memory_used_);

  // sample arrays of the master process consist of the three identical intervals:
  // 50 percentile worker processes, take 50 percentile of the interval => 17 percentile of all samples
  // 95 percentile worker processes, take 95 percentile of the interval => 65 percentile of all samples
  // 99 percentile worker processes, take 99 percentile of the interval => 99 percentile of all samples
  write_percentile<17, 65, 99>(stats, "requests.script_time", script_time_samples_);
  write_percentile<17, 65, 99>(stats, "requests.net_time", net_time_samples_);
  write_percentile<17, 65, 99>(stats, "requests.working_time", working_time_samples_);
  write_percentile<17, 65, 99>(stats, "memory.script_usage", script_memory_used_samples_);
  write_percentile<17, 65, 99>(stats, "memory.script_real_usage", script_real_memory_used_samples_);

  // here we use 1 sample from 1 worker, therefore no magic
  write_percentile<50, 95, 99>(stats, "memory.workers_script_heap_usage_bytes", workers_script_heap_usage_bytes_);
  write_percentile<50, 95, 99>(stats, "memory.workers_malloc_non_mapped_total_usage_bytes", workers_malloc_non_mapped_total_used_bytes_);
  write_percentile<50, 95, 99>(stats, "memory.workers_malloc_non_mapped_allocated_bytes", workers_malloc_non_mapped_allocated_bytes_);
  write_percentile<50, 95, 99>(stats, "memory.workers_malloc_non_mapped_free_bytes", workers_malloc_non_mapped_free_bytes_);
  write_percentile<50, 95, 99>(stats, "memory.workers_malloc_mapped_bytes", workers_malloc_mmaped_bytes_);
  write_percentile<50, 95, 99>(stats, "memory.workers_rss_bytes", workers_rss_usage_);
  write_percentile<50, 95, 99>(stats, "memory.workers_vms_bytes", workers_vms_usage_);
  write_percentile<50, 95, 99>(stats, "memory.workers_shm_bytes", workers_shm_usage_);

  add_gauge_stat_long(stats, "memory.master_malloc_non_mapped_allocated_bytes", internal_.malloc_stats_.total_non_mmaped_allocated_bytes_);
  add_gauge_stat_long(stats, "memory.master_malloc_non_mapped_free_bytes", internal_.malloc_stats_.total_non_mmaped_free_bytes_);
  add_gauge_stat_long(stats, "memory.master_malloc_mapped_bytes", internal_.malloc_stats_.total_mmaped_bytes_);

  add_gauge_stat_long(stats, "memory.master_rss_bytes", int64_t{internal_.mem_info_.rss} * 1024);
  add_gauge_stat_long(stats, "memory.master_vms_bytes", int64_t{internal_.mem_info_.vm} * 1024);
  add_gauge_stat_long(stats, "memory.master_shm_bytes", get_shm(internal_.mem_info_) * 1024);

  add_gauge_stat_long(stats, "memory.server_vms_max_bytes", int64_t{vms_kb_max_} * 1024);
  add_gauge_stat_long(stats, "memory.server_rss_max_bytes", int64_t{rss_kb_max_} * 1024);
  add_gauge_stat_long(stats, "memory.server_rss_no_shm_total_bytes", rss_no_shm_kb_sum_ * 1024);

  write_error_stat_to(stats, "terminated_requests.memory_limit_exceeded", script_error_t::memory_limit);
  write_error_stat_to(stats, "terminated_requests.timeout", script_error_t::timeout);
  write_error_stat_to(stats, "terminated_requests.exception", script_error_t::exception);
  write_error_stat_to(stats, "terminated_requests.stack_overflow", script_error_t::stack_overflow);
  write_error_stat_to(stats, "terminated_requests.php_assert", script_error_t::php_assert);
  write_error_stat_to(stats, "terminated_requests.http_connection_close", script_error_t::http_connection_close);
  write_error_stat_to(stats, "terminated_requests.rpc_connection_close", script_error_t::rpc_connection_close);
  write_error_stat_to(stats, "terminated_requests.net_event_error", script_error_t::net_event_error);
  write_error_stat_to(stats, "terminated_requests.post_data_loading_error", script_error_t::post_data_loading_error);
  write_error_stat_to(stats, "terminated_requests.unclassified", script_error_t::memory_limit);
}

int PhpWorkerStats::write_into(char *buffer, int buffer_len) const noexcept {
  static_assert(std::is_standard_layout<decltype(internal_)>{} , "PhpWorkerStats::internal_ is expected to be simple");
  constexpr size_t stats_size = sizeof(internal_);
  assert (buffer_len > stats_size);
  memcpy(buffer, &internal_, stats_size);
  return static_cast<int>(stats_size);
}

int PhpWorkerStats::read_from(const char *buffer) noexcept {
  static_assert(std::is_standard_layout<decltype(internal_)>{}, "PhpWorkerStats::internal_ is expected to be simple");
  internal_ = *reinterpret_cast<const decltype(internal_) *>(buffer);
  return static_cast<int>(sizeof(internal_));
}

void PhpWorkerStats::reset_memory_and_percentiles_stats() noexcept {
  internal_.script_max_memory_used_ = 0;
  internal_.script_max_real_memory_used_ = 0;

  request_circular_percentiles_counter_ = 0;
  internal_.working_time_percentiles_ = {};
  internal_.net_time_percentiles_ = {};
  internal_.script_time_percentiles_ = {};
  internal_.script_memory_used_percentiles_ = {};
  internal_.script_real_memory_used_percentiles_ = {};

  samples_tp_ = {};
  working_time_samples_ = {};
  net_time_samples_ = {};
  script_time_samples_ = {};
  script_memory_used_samples_ = {};
  script_real_memory_used_samples_ = {};

  worker_circular_percentiles_counter_ = 0;
  workers_script_heap_usage_bytes_ = {};
  workers_malloc_non_mapped_total_used_bytes_ = {};
  workers_malloc_non_mapped_allocated_bytes_ = {};
  workers_malloc_non_mapped_free_bytes_ = {};
  workers_malloc_mmaped_bytes_ = {};

  workers_rss_usage_ = {};
  workers_vms_usage_ = {};
  workers_shm_usage_ = {};

  rss_kb_sum_ = 0;
  vms_kb_sum_ = 0;

  rss_kb_max_ = 0;
  vms_kb_max_ = 0;

  rss_no_shm_kb_sum_ = 0;
}

void PhpWorkerStats::write_error_stat_to(stats_t *stats, const char *stat_name, script_error_t error) const noexcept {
  add_gauge_stat_long(stats, stat_name, internal_.errors_[static_cast<size_t>(error)]);
}
