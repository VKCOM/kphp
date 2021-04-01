// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "server/php-worker-stats.h"

#include <algorithm>
#include <cassert>
#include <cstring>

namespace {

template<class T>
void write_percentile(stats_t *stats, const char *prefix, std::array<T, 3> value) noexcept {
  add_gauge_stat(stats, value[0], prefix, ".percentile_50");
  add_gauge_stat(stats, value[1], prefix, ".percentile_95");
  add_gauge_stat(stats, value[2], prefix, ".percentile_99");
}

template<size_t P1, size_t P2, size_t P3, class T, size_t N>
std::array<T, 3> calc_percentiles(std::array<T, N> &samples) {
  const auto last = std::remove(samples.begin(), samples.end(), 0);
  const auto size = last - samples.begin();
  std::nth_element(samples.begin(), samples.begin() + P1 * size / 100, last);
  std::nth_element(samples.begin(), samples.begin() + P2 * size / 100, last);
  std::nth_element(samples.begin(), samples.begin() + P3 * size / 100, last);
  return std::array<T, 3>{samples[P1 * size / 100], samples[P2 * size / 100], samples[P3 * size / 100]};
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
} // namespace

void PhpWorkerStats::add_stats(double script_time, double net_time, long script_queries,
                               long max_memory_used, long max_real_memory_used, script_error_t error) noexcept {
  internal_.tot_queries_++;
  internal_.net_time_ += net_time;
  internal_.script_time_ += script_time;
  internal_.tot_script_queries_ += script_queries;
  internal_.script_max_memory_used_ = std::max(internal_.script_max_memory_used_, int64_t{max_memory_used});
  internal_.script_max_real_memory_used_ = std::max(internal_.script_max_real_memory_used_, int64_t{max_real_memory_used});
  ++internal_.errors_[static_cast<size_t>(error)];

  const size_t sample = circular_percentiles_counter_++;
  circular_percentiles_counter_ %= PERCENTILE_SAMPLES;
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

void PhpWorkerStats::recalc_worker_percentiles() noexcept {
  const auto now_tp = std::chrono::steady_clock::now();
  internal_.working_time_percentiles_ = calc_timed_50_95_99_percentiles(working_time_samples_, samples_tp_, now_tp);
  internal_.net_time_percentiles_ = calc_timed_50_95_99_percentiles(net_time_samples_, samples_tp_, now_tp);
  internal_.script_time_percentiles_ = calc_timed_50_95_99_percentiles(script_time_samples_, samples_tp_, now_tp);

  internal_.script_memory_used_percentiles_ = calc_timed_50_95_99_percentiles(script_memory_used_samples_, samples_tp_, now_tp);
  internal_.script_real_memory_used_percentiles_ = calc_timed_50_95_99_percentiles(script_real_memory_used_samples_, samples_tp_, now_tp);
}

void PhpWorkerStats::recalc_master_percentiles() noexcept {
  // sample arrays of the master process consist of the three identical intervals:
  // 50 percentile worker processes, take 50 percentile of the interval => 17 percentile of all samples
  // 95 percentile worker processes, take 95 percentile of the interval => 65 percentile of all samples
  // 99 percentile worker processes, take 99 percentile of the interval => 99 percentile of all samples
  internal_.working_time_percentiles_ = calc_percentiles<17, 65, 99>(working_time_samples_);
  internal_.net_time_percentiles_ = calc_percentiles<17, 65, 99>(net_time_samples_);
  internal_.script_time_percentiles_ = calc_percentiles<17, 65, 99>(script_time_samples_);

  internal_.script_memory_used_percentiles_ = calc_percentiles<17, 65, 99>(script_memory_used_samples_);
  internal_.script_real_memory_used_percentiles_ = calc_percentiles<17, 65, 99>(script_real_memory_used_samples_);
}

void PhpWorkerStats::add_from(const PhpWorkerStats &from) noexcept {
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

  const size_t offset = circular_percentiles_counter_;
  circular_percentiles_counter_ = (circular_percentiles_counter_ + PERCENTILES_COUNT) % PERCENTILE_SAMPLES;

  append_samples(from.internal_.working_time_percentiles_, working_time_samples_, offset);
  append_samples(from.internal_.net_time_percentiles_, net_time_samples_, offset);
  append_samples(from.internal_.script_time_percentiles_, script_time_samples_, offset);

  append_samples(from.internal_.script_memory_used_percentiles_, script_memory_used_samples_, offset);
  append_samples(from.internal_.script_real_memory_used_percentiles_, script_real_memory_used_samples_, offset);
}

void PhpWorkerStats::copy_internal_from(const PhpWorkerStats &from) noexcept {
  internal_ = from.internal_;
}

std::string PhpWorkerStats::to_string(const std::string &pid_s) const noexcept {
  char buf[1000] = {0};
  std::string res;

  const int cnt = std::max(internal_.accumulated_stats_, 1U);

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

void PhpWorkerStats::to_stats(stats_t *stats) const noexcept {
  add_gauge_stat_long(stats, "requests.total_incoming_queries", internal_.tot_queries_);
  add_gauge_stat_long(stats, "requests.total_outgoing_queries", internal_.tot_script_queries_);
  add_gauge_stat_double(stats, "requests.script_time.total", internal_.script_time_);
  write_percentile(stats, "requests.script_time", internal_.script_time_percentiles_);
  add_gauge_stat_double(stats, "requests.net_time.total", internal_.net_time_);
  write_percentile(stats, "requests.net_time", internal_.net_time_percentiles_);
  write_percentile(stats, "requests.working_time", internal_.working_time_percentiles_);

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

  add_gauge_stat_long(stats, "memory.script_usage.max", internal_.script_max_memory_used_);
  write_percentile(stats, "memory.script_usage", internal_.script_memory_used_percentiles_);
  add_gauge_stat_long(stats, "memory.script_real_usage.max", internal_.script_max_real_memory_used_);
  write_percentile(stats, "memory.script_real_usage", internal_.script_real_memory_used_percentiles_);
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

  circular_percentiles_counter_ = 0;
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
}

void PhpWorkerStats::write_error_stat_to(stats_t *stats, const char *stat_name, script_error_t error) const noexcept {
  add_gauge_stat_long(stats, stat_name, internal_.errors_[static_cast<size_t>(error)]);
}
