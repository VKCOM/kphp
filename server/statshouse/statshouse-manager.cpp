// Compiler for PHP (aka KPHP)
// Copyright (c) 2023 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "server/statshouse/statshouse-manager.h"

#include <chrono>

#include "common/precise-time.h"
#include "common/resolver.h"
#include "runtime/instance-cache.h"
#include "server/job-workers/shared-memory-manager.h"
#include "server/confdata-stats.h"
#include "server/json-logger.h"
#include "server/php-runner.h"
#include "server/server-config.h"
#include "server/server-stats.h"
#include "server/shared-data.h"

namespace {
template<typename T>
T unpack(const std::atomic<T> &value) {
  return value.load(std::memory_order_relaxed);
}

inline size_t get_memory_used(size_t acquired, size_t released, size_t buffer_size) {
  return acquired > released ? (acquired - released) * buffer_size : 0;
}

const char *get_current_worker_type() {
  switch (process_type) {
    case ProcessType::http_worker: return "http";
    case ProcessType::rpc_worker:  return "rpc";
    case ProcessType::job_worker:  return "job";
    default: return "";
  }
}

const char *script_error_to_str(script_error_t error) {
  switch (error) {
    case script_error_t::no_error:                return "ok";
    case script_error_t::memory_limit:            return "memory_limit";
    case script_error_t::timeout:                 return "timeout";
    case script_error_t::exception:               return "exception";
    case script_error_t::stack_overflow:          return "stack_overflow";
    case script_error_t::php_assert:              return "php_assert";
    case script_error_t::http_connection_close:   return "http_connection_close";
    case script_error_t::rpc_connection_close:    return "rpc_connection_close";
    case script_error_t::net_event_error:         return "net_event_error";
    case script_error_t::post_data_loading_error: return "post_data_loading_error";
    default:                                      return "unclassified_error";
  }
}
} // namespace

StatsHouseManager::StatsHouseManager(const std::string &ip, int port)
  : client(ip, port){};

void StatsHouseManager::set_common_tags() {
  client.set_tag_cluster(vk::singleton<ServerConfig>::get().get_cluster_name());
  client.set_tag_host(kdb_gethostname());
}

void StatsHouseManager::generic_cron_check_if_tag_host_needed() {
  using namespace std::chrono_literals;

  static SharedData::time_point last_check_tp;
  auto now_tp = SharedData::clock::now();

  // check is done every 5 sec
  if (now_tp - last_check_tp < 5s) {
    return;
  }
  last_check_tp = now_tp;

  auto &shared_data = vk::singleton<SharedData>::get();

  if (need_write_enable_tag_host) {
    // if php script called kphp_turn_on_host_tag_in_inner_statshouse_metrics_toggle()
    // we store current time in shared memory to make other processes enable host tag.
    // NB: so far it never happens in master process
    shared_data.store_start_use_host_in_statshouse_metrics_timestamp(now_tp);
    need_write_enable_tag_host = false;
    client.enable_tag_host();
    return;
  }

  // then every process checks the timestamp from shared memory to decide whether to enable host tag or not
  auto tp = shared_data.load_start_use_host_in_statshouse_metrics_timestamp();
  if (now_tp - tp < 30s) {
    client.enable_tag_host();
  } else {
    // if 30 secs has already passed, disable tag host
    client.disable_tag_host();
  }
}

void StatsHouseManager::add_request_stats(uint64_t script_time_ns, uint64_t net_time_ns, script_error_t error,
                                          const memory_resource::MemoryStats &script_memory_stats, uint64_t script_queries, uint64_t long_script_queries,
                                          uint64_t script_user_time_ns, uint64_t script_system_time_ns,
                                          uint64_t script_init_time, uint64_t http_connection_process_time,
                                          uint64_t voluntary_context_switches, uint64_t involuntary_context_switches) {
  const char *worker_type = get_current_worker_type();
  const char *status = script_error_to_str(error);

  client.metric("kphp_request_time").tag("script").tag(worker_type).tag(status).write_value(script_time_ns);
  client.metric("kphp_request_time").tag("net").tag(worker_type).tag(status).write_value(net_time_ns);
  client.metric("kphp_request_cpu_time").tag("user").tag(worker_type).tag(status).write_value(script_user_time_ns);
  client.metric("kphp_request_cpu_time").tag("system").tag(worker_type).tag(status).write_value(script_system_time_ns);
  client.metric("kphp_request_init_time").tag(worker_type).tag(status).write_value(script_init_time);
  if (process_type == ProcessType::http_worker) {
    client.metric("kphp_http_connection_process_time").tag(status).write_value(http_connection_process_time);
  }

  client.metric("kphp_by_host_request_time", true).tag("script").tag(worker_type).write_value(script_time_ns);
  client.metric("kphp_by_host_request_time", true).tag("net").tag(worker_type).write_value(net_time_ns);
  client.metric("kphp_by_host_request_cpu_time", true).tag("user").tag(worker_type).tag(status).write_value(script_user_time_ns);
  client.metric("kphp_by_host_request_cpu_time", true).tag("system").tag(worker_type).tag(status).write_value(script_system_time_ns);
  client.metric("kphp_by_host_request_init_time", true).tag(worker_type).tag(status).write_value(script_init_time);
  if (process_type == ProcessType::http_worker) {
    client.metric("kphp_by_host_http_connection_process_time", true).tag(status).write_value(http_connection_process_time);
  }

  if (error != script_error_t::no_error) {
    client.metric("kphp_request_errors").tag(status).tag(worker_type).write_count(1);
    client.metric("kphp_by_host_request_errors", true).tag(status).tag(worker_type).write_count(1);
  }

  client.metric("kphp_memory_script_usage").tag("used").tag(worker_type).write_value(script_memory_stats.memory_used);
  client.metric("kphp_memory_script_usage").tag("real_used").tag(worker_type).write_value(script_memory_stats.real_memory_used);

  client.metric("kphp_memory_script_allocated_total").tag(worker_type).write_value(script_memory_stats.total_memory_allocated);
  client.metric("kphp_memory_script_allocations_count").tag(worker_type).write_value(script_memory_stats.total_allocations);

  client.metric("kphp_requests_outgoing_queries").tag(worker_type).write_value(script_queries);
  client.metric("kphp_requests_outgoing_long_queries").tag(worker_type).write_value(long_script_queries);

  client.metric("kphp_request_scheduler_context_switches").tag("voluntary").tag(status).write_value(voluntary_context_switches);
  client.metric("kphp_request_scheduler_context_switches").tag("involuntary").tag(status).write_value(involuntary_context_switches);

  client.metric("kphp_by_host_request_scheduler_context_switches", true).tag("voluntary").tag(status).write_value(voluntary_context_switches);
  client.metric("kphp_by_host_request_scheduler_context_switches", true).tag("involuntary").tag(status).write_value(involuntary_context_switches);
}

void StatsHouseManager::add_job_stats(uint64_t job_wait_ns, uint64_t request_memory_used, uint64_t request_real_memory_used, uint64_t response_memory_used,
                                     uint64_t response_real_memory_used) {
  client.metric("kphp_job_queue_time").write_value(job_wait_ns);

  client.metric("kphp_job_request_memory_usage").tag("used").write_value(request_memory_used);
  client.metric("kphp_job_request_memory_usage").tag("real_used").write_value(request_real_memory_used);

  client.metric("kphp_job_response_memory_usage").tag("used").write_value(response_memory_used);
  client.metric("kphp_job_response_memory_usage").tag("real_used").write_value(response_real_memory_used);
}

void StatsHouseManager::add_job_common_memory_stats(uint64_t job_common_request_memory_used, uint64_t job_common_request_real_memory_used) {
  dl::CriticalSectionGuard guard; // It's called from script context, so we need to ensure SIGALRM won't interrupt us here
  client.metric("kphp_job_common_request_memory").tag("used").write_value(job_common_request_memory_used);
  client.metric("kphp_job_common_request_memory").tag("real_used").write_value(job_common_request_real_memory_used);
}

void StatsHouseManager::add_worker_memory_stats(const mem_info_t &mem_stats) {
  const char *worker_type = get_current_worker_type();
  client.metric("kphp_workers_memory").tag(worker_type).tag("vm_peak").write_value(mem_stats.vm_peak);
  client.metric("kphp_workers_memory").tag(worker_type).tag("vm").write_value(mem_stats.vm);
  client.metric("kphp_workers_memory").tag(worker_type).tag("rss").write_value(mem_stats.rss);
  client.metric("kphp_workers_memory").tag(worker_type).tag("rss_peak").write_value(mem_stats.rss_peak);

  client.metric("kphp_by_host_workers_memory", true).tag(worker_type).tag("vm_peak").write_value(mem_stats.vm_peak);
  client.metric("kphp_by_host_workers_memory", true).tag(worker_type).tag("vm").write_value(mem_stats.vm);
  client.metric("kphp_by_host_workers_memory", true).tag(worker_type).tag("rss").write_value(mem_stats.rss);
  client.metric("kphp_by_host_workers_memory", true).tag(worker_type).tag("rss_peak").write_value(mem_stats.rss_peak);
}

void StatsHouseManager::add_common_master_stats(const workers_stats_t &workers_stats,
                                                const memory_resource::MemoryStats &instance_cache_memory_stats,
                                                double cpu_s_usage, double cpu_u_usage,
                                                long long int instance_cache_memory_swaps_ok, long long int instance_cache_memory_swaps_fail) {
  if (engine_tag) {
    client.metric("kphp_version").write_value(atoll(engine_tag));
  }

  client.metric("kphp_uptime").write_value(get_uptime());

  const auto general_worker_group = vk::singleton<ServerStats>::get().collect_workers_stat(WorkerType::general_worker);
  client.metric("kphp_workers_general_processes").tag("working").write_value(general_worker_group.running_workers);
  client.metric("kphp_workers_general_processes").tag("working_but_waiting").write_value(general_worker_group.waiting_workers);
  client.metric("kphp_workers_general_processes").tag("ready_for_accept").write_value(general_worker_group.ready_for_accept_workers);

  const auto job_worker_group = vk::singleton<ServerStats>::get().collect_workers_stat(WorkerType::job_worker);
  client.metric("kphp_workers_job_processes").tag("working").write_value(job_worker_group.running_workers);
  client.metric("kphp_workers_job_processes").tag("working_but_waiting").write_value(job_worker_group.waiting_workers);

  client.metric("kphp_server_workers").tag("started").write_value(workers_stats.tot_workers_started);
  client.metric("kphp_server_workers").tag("dead").write_value(workers_stats.tot_workers_dead);
  client.metric("kphp_server_workers").tag("strange_dead").write_value(workers_stats.tot_workers_strange_dead);
  client.metric("kphp_server_workers").tag("killed").write_value(workers_stats.workers_killed);
  client.metric("kphp_server_workers").tag("hung").write_value(workers_stats.workers_hung);
  client.metric("kphp_server_workers").tag("terminated").write_value(workers_stats.workers_terminated);
  client.metric("kphp_server_workers").tag("failed").write_value(workers_stats.workers_failed);

  client.metric("kphp_cpu_usage").tag("sys").write_value(cpu_s_usage);
  client.metric("kphp_cpu_usage").tag("user").write_value(cpu_u_usage);

  client.metric("kphp_by_host_cpu_usage", true).tag("sys").write_value(cpu_s_usage);
  client.metric("kphp_by_host_cpu_usage", true).tag("user").write_value(cpu_u_usage);

  auto total_workers_json_count = vk::singleton<ServerStats>::get().collect_json_count_stat();
  uint64_t master_json_logs_count = vk::singleton<JsonLogger>::get().get_json_logs_count();
  client.metric("kphp_server_total_json_logs_count").write_value(std::get<0>(total_workers_json_count) + master_json_logs_count);
  client.metric("kphp_server_total_json_traces_count").write_value(std::get<1>(total_workers_json_count));

  client.metric("kphp_instance_cache_memory").tag("limit").write_value(instance_cache_memory_stats.memory_limit);
  client.metric("kphp_instance_cache_memory").tag("used").write_value(instance_cache_memory_stats.memory_used);
  client.metric("kphp_instance_cache_memory").tag("real_used").write_value(instance_cache_memory_stats.real_memory_used);

  client.metric("kphp_instance_cache_memory_defragmentation_calls").write_value(instance_cache_memory_stats.defragmentation_calls);

  client.metric("kphp_instance_cache_memory_pieces").tag("huge").write_value(instance_cache_memory_stats.huge_memory_pieces);
  client.metric("kphp_instance_cache_memory_pieces").tag("small").write_value(instance_cache_memory_stats.small_memory_pieces);

  client.metric("kphp_instance_cache_memory_buffer_swaps").tag("ok").write_value(instance_cache_memory_swaps_ok);
  client.metric("kphp_instance_cache_memory_buffer_swaps").tag("fail").write_value(instance_cache_memory_swaps_fail);

  const auto &instance_cache_element_stats = instance_cache_get_stats();
  client.metric("kphp_instance_cache_elements").tag("stored").write_value(unpack(instance_cache_element_stats.elements_stored));
  client.metric("kphp_instance_cache_elements").tag("stored_with_delay").write_value(unpack(instance_cache_element_stats.elements_stored_with_delay));
  client.metric("kphp_instance_cache_elements").tag("storing_skipped_due_recent_update").write_value(unpack(instance_cache_element_stats.elements_storing_skipped_due_recent_update));
  client.metric("kphp_instance_cache_elements").tag("storing_delayed_due_mutex").write_value(unpack(instance_cache_element_stats.elements_storing_delayed_due_mutex));
  client.metric("kphp_instance_cache_elements").tag("fetched").write_value(unpack(instance_cache_element_stats.elements_fetched));
  client.metric("kphp_instance_cache_elements").tag("missed").write_value(unpack(instance_cache_element_stats.elements_missed));
  client.metric("kphp_instance_cache_elements").tag("missed_earlier").write_value(unpack(instance_cache_element_stats.elements_missed_earlier));
  client.metric("kphp_instance_cache_elements").tag("expired").write_value(unpack(instance_cache_element_stats.elements_expired));
  client.metric("kphp_instance_cache_elements").tag("created").write_value(unpack(instance_cache_element_stats.elements_created));
  client.metric("kphp_instance_cache_elements").tag("destroyed").write_value(unpack(instance_cache_element_stats.elements_destroyed));
  client.metric("kphp_instance_cache_elements").tag("cached").write_value(unpack(instance_cache_element_stats.elements_cached));
  client.metric("kphp_instance_cache_elements").tag("logically_expired_and_ignored").write_value(unpack(instance_cache_element_stats.elements_logically_expired_and_ignored));
  client.metric("kphp_instance_cache_elements").tag("logically_expired_but_fetched").write_value(unpack(instance_cache_element_stats.elements_logically_expired_but_fetched));

  using namespace job_workers;
  if (vk::singleton<job_workers::SharedMemoryManager>::get().is_initialized()) {
    const JobStats &job_stats = vk::singleton<SharedMemoryManager>::get().get_stats();
    client.metric("kphp_workers_jobs_queue_size").write_value(unpack(job_stats.job_queue_size));
    this->add_job_workers_shared_memory_stats(job_stats);
  }
}

void StatsHouseManager::add_init_master_stats(uint64_t total_init_ns, uint64_t confdata_init_ns) {
  client.metric("kphp_by_host_master_total_init_time", true).write_value(total_init_ns);
  client.metric("kphp_by_host_master_confdata_init_time", true).write_value(confdata_init_ns);
}

void StatsHouseManager::add_extended_instance_cache_stats(std::string_view type, std::string_view status, const string &key, uint64_t size) {
  dl::CriticalSectionGuard guard;
  string normalize_key = instance_cache_key_normalization_function(key);
  client.metric("kphp_instance_cache_data_size", true).tag(type).tag(status).tag(normalize_key.c_str()).write_value(size);
}

void StatsHouseManager::add_job_workers_shared_memory_stats(const job_workers::JobStats &job_stats) {
  using namespace job_workers;

  size_t total_used = this->add_job_workers_shared_messages_stats(job_stats.messages, JOB_SHARED_MESSAGE_BYTES);

  constexpr std::array<const char *, JOB_EXTRA_MEMORY_BUFFER_BUCKETS> extra_memory_prefixes{
    "256kb", "512kb", "1mb", "2mb", "4mb", "8mb", "16mb", "32mb", "64mb",
  };
  for (size_t i = 0; i != JOB_EXTRA_MEMORY_BUFFER_BUCKETS; ++i) {
    const size_t buffer_size = get_extra_shared_memory_buffer_size(i);
    total_used += this->add_job_workers_shared_memory_buffers_stats(job_stats.extra_memory[i], extra_memory_prefixes[i], buffer_size);
  }

  client.metric("kphp_job_workers_shared_memory").tag("limit").write_value(job_stats.memory_limit);
  client.metric("kphp_job_workers_shared_memory").tag("used").write_value(total_used);
}

size_t StatsHouseManager::add_job_workers_shared_messages_stats(const job_workers::JobStats::MemoryBufferStats &memory_buffers_stats,
                                                               size_t buffer_size) {
  using namespace job_workers;

  const size_t acquired_buffers = unpack(memory_buffers_stats.acquired);
  const size_t released_buffers = unpack(memory_buffers_stats.released);
  const size_t memory_used = get_memory_used(acquired_buffers, released_buffers, buffer_size);

  client.metric("kphp_job_workers_shared_messages").tag("reserved").write_value(memory_buffers_stats.count);
  client.metric("kphp_job_workers_shared_messages").tag("acquire_fails").write_value(unpack(memory_buffers_stats.acquire_fails));
  client.metric("kphp_job_workers_shared_messages").tag("acquired").write_value(acquired_buffers);
  client.metric("kphp_job_workers_shared_messages").tag("released").write_value(released_buffers);

  return memory_used;
}

size_t StatsHouseManager::add_job_workers_shared_memory_buffers_stats(const job_workers::JobStats::MemoryBufferStats &memory_buffers_stats, const char *size_tag,
                                                                     size_t buffer_size) {
  using namespace job_workers;

  const size_t acquired_buffers = unpack(memory_buffers_stats.acquired);
  const size_t released_buffers = unpack(memory_buffers_stats.released);
  const size_t memory_used = get_memory_used(acquired_buffers, released_buffers, buffer_size);

  client.metric("kphp_job_workers_shared_extra_buffers").tag(size_tag).tag("reserved").write_value(memory_buffers_stats.count);
  client.metric("kphp_job_workers_shared_extra_buffers").tag(size_tag).tag("acquire_fails").write_value(unpack(memory_buffers_stats.acquire_fails));
  client.metric("kphp_job_workers_shared_extra_buffers").tag(size_tag).tag("acquired").write_value(acquired_buffers);
  client.metric("kphp_job_workers_shared_extra_buffers").tag(size_tag).tag("released").write_value(released_buffers);

  return memory_used;
}

void StatsHouseManager::add_confdata_master_stats(const ConfdataStats &confdata_stats) {
  const auto &memory_stats = confdata_stats.get_memory_stats();
  client.metric("kphp_confdata_memory").tag("limit").write_value(memory_stats.memory_limit);
  client.metric("kphp_confdata_memory").tag("used").write_value(memory_stats.memory_used);
  client.metric("kphp_confdata_memory").tag("real_used").write_value(memory_stats.real_memory_used);

  const auto &events = confdata_stats.event_counters;
  client.metric("kphp_confdata_events").tag("set").write_value(events.set_events.total + events.set_forever_events.total);
  client.metric("kphp_confdata_events").tag("set_blacklisted").write_value(events.set_events.blacklisted + events.set_forever_events.blacklisted);
  client.metric("kphp_confdata_events").tag("delete").write_value(events.delete_events.total);
  client.metric("kphp_confdata_events").tag("delete_blacklisted").write_value(events.delete_events.blacklisted);
  client.metric("kphp_confdata_events").tag("throttled_out").write_value(events.throttled_out_total_events);

  for (const auto &[section_name, size] : confdata_stats.heaviest_sections_by_count.sorted_desc) {
    if (section_name != nullptr && size > 0) { // section_name looks like "highload."
      client.metric("kphp_confdata_sections_by_count").tag(section_name->c_str()).write_value(size);
      // fprintf(stderr, "%s: %d\n", section_name->c_str(), static_cast<int>(size));
    }
  }
}
