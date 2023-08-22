// Compiler for PHP (aka KPHP)
// Copyright (c) 2023 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#include "server/statshouse/statshouse-client.h"

#include "common/precise-time.h"
#include "runtime/instance-cache.h"
#include "server/job-workers/job-stats.h"
#include "server/job-workers/shared-memory-manager.h"
#include "server/json-logger.h"
#include "server/server-config.h"
#include "server/server-stats.h"
#include "server/workers-stats.h"

StatsHouseClient *StatsHouseClient::inner = nullptr;

StatsHouseClient::StatsHouseClient(const std::string &ip, int port)
  : transport(ip, port){};

void StatsHouseClient::add_request_stats(WorkerType raw_worker_type, uint64_t script_time_ns, uint64_t net_time_ns, uint64_t memory_used,
                                         uint64_t real_memory_used, uint64_t script_queries, uint64_t long_script_queries) {
  const char *cluster_name = vk::singleton<ServerConfig>::get().get_cluster_name();
  const char *worker_type = raw_worker_type == WorkerType::general_worker ? "general" : "job";
  transport.metric("kphp_request_time").tag(cluster_name).tag("script").tag(worker_type).write_value(script_time_ns);
  transport.metric("kphp_request_time").tag(cluster_name).tag("net").tag(worker_type).write_value(net_time_ns);

  transport.metric("kphp_memory_script_usage").tag(cluster_name).tag("used").tag(worker_type).write_value(memory_used);
  transport.metric("kphp_memory_script_usage").tag(cluster_name).tag("free").tag(worker_type).write_value(real_memory_used);

  transport.metric("kphp_requests_outgoing_queries").tag(cluster_name).tag(worker_type).write_value(script_queries);
  transport.metric("kphp_requests_outgoing_long_queries").tag(cluster_name).tag(worker_type).write_value(long_script_queries);
}

void StatsHouseClient::add_job_stats(uint64_t job_wait_ns, uint64_t request_memory_used, uint64_t request_real_memory_used, uint64_t response_memory_used,
                                     uint64_t response_real_memory_used) {
  const char *cluster_name = vk::singleton<ServerConfig>::get().get_cluster_name();
  transport.metric("kphp_job_queue_time").tag(cluster_name).write_value(job_wait_ns);

  transport.metric("kphp_job_request_memory_usage").tag(cluster_name).tag("used").write_value(request_memory_used);
  transport.metric("kphp_job_request_memory_usage").tag(cluster_name).tag("real_used").write_value(request_real_memory_used);

  transport.metric("kphp_job_response_memory_usage").tag(cluster_name).tag("used").write_value(response_memory_used);
  transport.metric("kphp_job_response_memory_usage").tag(cluster_name).tag("real_used").write_value(response_real_memory_used);
}

void StatsHouseClient::add_job_common_memory_stats(uint64_t job_common_request_memory_used, uint64_t job_common_request_real_memory_used) {
  const char *cluster_name = vk::singleton<ServerConfig>::get().get_cluster_name();
  transport.metric("kphp_job_common_request_memory").tag(cluster_name).tag("used").write_value(job_common_request_memory_used);
  transport.metric("kphp_job_common_request_memory").tag(cluster_name).tag("real_used").write_value(job_common_request_real_memory_used);
}

void StatsHouseClient::add_common_master_stats(const workers_stats_t &workers_stats, const memory_resource::MemoryStats &memory_stats,
                                               double cpu_s_usage, double cpu_u_usage,
                                               long long int instance_cache_memory_swaps_ok, long long int instance_cache_memory_swaps_fail) {
  const char *cluster_name = vk::singleton<ServerConfig>::get().get_cluster_name();
  if (engine_tag) {
    transport.metric("kphp_version").tag(cluster_name).write_value(atoll(engine_tag));
  }

  transport.metric("kphp_uptime").tag(cluster_name).write_value(get_uptime());

  const auto general_worker_group = vk::singleton<ServerStats>::get().collect_workers_stat(WorkerType::general_worker);
  transport.metric("kphp_workers_general_processes").tag(cluster_name).tag("working").write_value(general_worker_group.running_workers);
  transport.metric("kphp_workers_general_processes").tag(cluster_name).tag("working_but_waiting").write_value(general_worker_group.waiting_workers);
  transport.metric("kphp_workers_general_processes").tag(cluster_name).tag("ready_for_accept").write_value(general_worker_group.ready_for_accept_workers);

  const auto job_worker_group = vk::singleton<ServerStats>::get().collect_workers_stat(WorkerType::job_worker);
  transport.metric("kphp_workers_job_processes").tag(cluster_name).tag("working").write_value(job_worker_group.running_workers);
  transport.metric("kphp_workers_job_processes").tag(cluster_name).tag("working_but_waiting").write_value(job_worker_group.waiting_workers);

  transport.metric("kphp_server_workers").tag(cluster_name).tag("started").write_value(workers_stats.tot_workers_started);
  transport.metric("kphp_server_workers").tag(cluster_name).tag("dead").write_value(workers_stats.tot_workers_dead);
  transport.metric("kphp_server_workers").tag(cluster_name).tag("strange_dead").write_value(workers_stats.tot_workers_strange_dead);
  transport.metric("kphp_server_workers").tag(cluster_name).tag("killed").write_value(workers_stats.workers_killed);
  transport.metric("kphp_server_workers").tag(cluster_name).tag("hung").write_value(workers_stats.workers_hung);
  transport.metric("kphp_server_workers").tag(cluster_name).tag("terminated").write_value(workers_stats.workers_terminated);
  transport.metric("kphp_server_workers").tag(cluster_name).tag("failed").write_value(workers_stats.workers_failed);

  transport.metric("kphp_cpu_usage").tag(cluster_name).tag("stime").write_value(cpu_s_usage);
  transport.metric("kphp_cpu_usage").tag(cluster_name).tag("utime").write_value(cpu_u_usage);

  auto total_workers_json_count = vk::singleton<ServerStats>::get().collect_json_count_stat();
  uint64_t master_json_logs_count = vk::singleton<JsonLogger>::get().get_json_logs_count();
  transport.metric("kphp_server_total_json_logs_count").tag(cluster_name).write_value(std::get<0>(total_workers_json_count) + master_json_logs_count);
  transport.metric("kphp_server_total_json_traces_count").tag(cluster_name).write_value(std::get<1>(total_workers_json_count));

  transport.metric("kphp_instance_cache_memory").tag(cluster_name).tag("limit").write_value(memory_stats.memory_limit);
  transport.metric("kphp_instance_cache_memory").tag(cluster_name).tag("used").write_value(memory_stats.memory_used);
  transport.metric("kphp_instance_cache_memory").tag(cluster_name).tag("real_used").write_value(memory_stats.real_memory_used);

  transport.metric("kphp_instance_cache_memory_defragmentation_calls").tag(cluster_name).write_value(memory_stats.defragmentation_calls);

  transport.metric("kphp_instance_cache_memory_pieces").tag(cluster_name).tag("huge").write_value(memory_stats.huge_memory_pieces);
  transport.metric("kphp_instance_cache_memory_pieces").tag(cluster_name).tag("small").write_value(memory_stats.small_memory_pieces);

  transport.metric("kphp_instance_cache_memory_buffer_swaps").tag(cluster_name).tag("ok").write_value(instance_cache_memory_swaps_ok);
  transport.metric("kphp_instance_cache_memory_buffer_swaps").tag(cluster_name).tag("fail").write_value(instance_cache_memory_swaps_fail);

  const auto &instance_cache_element_stats = instance_cache_get_stats();
  transport.metric("kphp_instance_cache_elements").tag(cluster_name).tag("stored").write_value(instance_cache_element_stats.elements_stored);
  transport.metric("kphp_instance_cache_elements")
    .tag(cluster_name)
    .tag("stored_with_delay")
    .write_value(instance_cache_element_stats.elements_stored_with_delay);
  transport.metric("kphp_instance_cache_elements")
    .tag(cluster_name)
    .tag("storing_skipped_due_recent_update")
    .write_value(instance_cache_element_stats.elements_storing_skipped_due_recent_update);
  transport.metric("kphp_instance_cache_elements")
    .tag(cluster_name)
    .tag("storing_delayed_due_mutex")
    .write_value(instance_cache_element_stats.elements_storing_delayed_due_mutex);
  transport.metric("kphp_instance_cache_elements").tag(cluster_name).tag("fetched").write_value(instance_cache_element_stats.elements_fetched);
  transport.metric("kphp_instance_cache_elements").tag(cluster_name).tag("missed").write_value(instance_cache_element_stats.elements_missed);
  transport.metric("kphp_instance_cache_elements").tag(cluster_name).tag("missed_earlier").write_value(instance_cache_element_stats.elements_missed_earlier);
  transport.metric("kphp_instance_cache_elements").tag(cluster_name).tag("expired").write_value(instance_cache_element_stats.elements_expired);
  transport.metric("kphp_instance_cache_elements").tag(cluster_name).tag("created").write_value(instance_cache_element_stats.elements_created);
  transport.metric("kphp_instance_cache_elements").tag(cluster_name).tag("destroyed").write_value(instance_cache_element_stats.elements_destroyed);
  transport.metric("kphp_instance_cache_elements").tag(cluster_name).tag("cached").write_value(instance_cache_element_stats.elements_cached);
  transport.metric("kphp_instance_cache_elements")
    .tag(cluster_name)
    .tag("logically_expired_and_ignored")
    .write_value(instance_cache_element_stats.elements_logically_expired_and_ignored);
  transport.metric("kphp_instance_cache_elements")
    .tag(cluster_name)
    .tag("logically_expired_but_fetched")
    .write_value(instance_cache_element_stats.elements_logically_expired_but_fetched);
}
