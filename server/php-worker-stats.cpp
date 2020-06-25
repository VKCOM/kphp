#include "server/php-worker-stats.h"

#include <cassert>
#include <cstring>

void PhpWorkerStats::add_stats(double script_time, double net_time, long script_queries,
                               long max_memory_used, long max_real_memory_used, script_error_t error) noexcept {
  internal_.tot_queries_++;
  internal_.worked_time_ += script_time + net_time;
  internal_.net_time_ += net_time;
  internal_.script_time_ += script_time;
  internal_.tot_script_queries_ += script_queries;
  internal_.script_max_memory_used_ = std::max(internal_.script_max_memory_used_, max_memory_used);
  internal_.script_max_real_memory_used_ = std::max(internal_.script_max_real_memory_used_, max_real_memory_used);
  ++internal_.errors_[static_cast<size_t>(error)];
}

void PhpWorkerStats::update_idle_time(double tot_idle_time, int uptime, double average_idle_time, double average_idle_quotient) noexcept {
  internal_.tot_idle_time_ = tot_idle_time;
  internal_.tot_idle_percent_ = uptime > 0 ? tot_idle_time / uptime * 100 : 0;
  internal_.a_idle_percent_ = average_idle_quotient > 0 ? average_idle_time / average_idle_quotient * 100 : 0;
}

void PhpWorkerStats::add_from(const PhpWorkerStats &from) noexcept {
  internal_.tot_queries_ += from.internal_.tot_queries_;
  internal_.worked_time_ += from.internal_.worked_time_;
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
}

std::string PhpWorkerStats::to_string(const std::string &pid_s) const noexcept {
  char buf[1000] = {0};
  std::string res;

  const int cnt = std::max(internal_.accumulated_stats_, 1);

  sprintf(buf, "tot_queries%s\t%ld\n", pid_s.c_str(), internal_.tot_queries_);
  res += buf;
  sprintf(buf, "worked_time%s\t%.3lf\n", pid_s.c_str(), internal_.worked_time_);
  res += buf;
  sprintf(buf, "net_time%s\t%.3lf\n", pid_s.c_str(), internal_.net_time_);
  res += buf;
  sprintf(buf, "script_time%s\t%.3lf\n", pid_s.c_str(), internal_.script_time_);
  res += buf;
  sprintf(buf, "tot_script_queries%s\t%ld\n", pid_s.c_str(), internal_.tot_script_queries_);
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
  add_histogram_stat_long(stats, "requests.total_incoming_queries", internal_.tot_queries_);
  add_histogram_stat_long(stats, "requests.total_outgoing_queries", internal_.tot_script_queries_);
  add_histogram_stat_double(stats, "requests.script_time", internal_.script_time_);
  add_histogram_stat_double(stats, "requests.net_time", internal_.net_time_);

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

  add_histogram_stat_long(stats, "memory.script_usage_max", internal_.script_max_memory_used_);
  add_histogram_stat_long(stats, "memory.script_real_usage_max", internal_.script_max_real_memory_used_);
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

void PhpWorkerStats::reset_memory_usage() noexcept {
  internal_.script_max_memory_used_ = 0;
  internal_.script_max_real_memory_used_ = 0;
}

void PhpWorkerStats::write_error_stat_to(stats_t *stats, const char *stat_name, script_error_t error) const noexcept {
  add_histogram_stat_long(stats, stat_name, internal_.errors_[static_cast<size_t>(error)]);
}
