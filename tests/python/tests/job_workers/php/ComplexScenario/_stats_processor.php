<?php

use \ComplexScenario\NetPid;
use ComplexScenario\StatTraits;
use ComplexScenario\Stats;

function mc_get(string $key, int $master_port) {
  static $mc = null;
  if ($mc === null) {
    $mc = new McMemcache();
    $mc->addServer("localhost", $master_port);
  }
  sched_yield();
  return $mc->get($key);
}

function rpc_query(array $request, int $master_port) {
  static $connection = null;
  if ($connection === null) {
    $connection = new_rpc_connection("localhost", $master_port);
  }
  return rpc_tl_query_one($connection, $request);
}

function start_stats_processing(StatTraits $traits, int $master_port) {
  switch($traits->stat_type) {
    case StatTraits::MC_STATS:
      return mc_get("stats", $master_port);
    case StatTraits::MC_STATS_FULL:
      return mc_get("stats_full", $master_port);
    case StatTraits::MC_STATS_FAST:
      return mc_get("stats_fast", $master_port);
    case StatTraits::RPC_STATS:
      return rpc_query(['_' => 'engine.stat'], $master_port);
    case StatTraits::RPC_FILTERED_STATS:
      return rpc_query(['_' => 'engine.filteredStat', 'stat_names' => $traits->stat_names], $master_port);
  }
  critical_error("Unknown stat type");
  return null;
}

function add_stat_to(Stats $stats, array $allowed_stat_names, $key, $value) {
  $key = trim($key);
  if (!preg_match("/^[a-zA-Z]+$/", $key)) {
    return;
  }
  $value = trim($value);
  if (empty($allowed_stat_names) || in_array($key, $allowed_stat_names)) {
    $stats->add_stat($key, $value);
  }
}

function parse_mc_stats(string $stats_raw, array $allowed_stat_names) : Stats {
  $stats = new Stats();
  foreach (explode("\n", $stats_raw) as $stat_line) {
    [$key, $value] = preg_split('#\s+#', $stat_line, 2);
    add_stat_to($stats, $allowed_stat_names, $key, $value);
  }
  $stats->normalize();
  return $stats;
}

function parse_rpc_stats(array $stats_raw, array $allowed_stat_names = []) : Stats {
  $stats = new Stats();
  foreach ($stats_raw["result"] as $key => $value) {
    add_stat_to($stats, $allowed_stat_names, $key, $value);
  }
  $stats->normalize();
  return $stats;
}

function finish_stats_processing(StatTraits $traits, $processing_id) : Stats {
  switch($traits->stat_type) {
    case StatTraits::MC_STATS:
    case StatTraits::MC_STATS_FULL:
    case StatTraits::MC_STATS_FAST:
      return parse_mc_stats((string)$processing_id, $traits->stat_names);
    case StatTraits::RPC_STATS:
      $result = rpc_tl_query_result_one($processing_id);
      return parse_rpc_stats($result, $traits->stat_names);
    case StatTraits::RPC_FILTERED_STATS:
      $result = rpc_tl_query_result_one($processing_id);
      return parse_rpc_stats($result);
  }
  critical_error("Unknown stat type");
  return new Stats();
}

function get_worker_pids(int $master_port): array {
  $pids_raw = mc_get("workers_pids", $master_port);
  return array_map("intval", explode(",", $pids_raw));
}

function start_net_pid_processing(int $master_port) : int {
  return rpc_query(['_' => 'engine.pid'], $master_port);
}

function finish_net_pid_processing(int $processing_id) : NetPid {
  $net_pid_result = rpc_tl_query_result_one($processing_id);
  $net_pid_result = $net_pid_result["result"];
  $net_pid = new NetPid;
  $net_pid->ip = (int)$net_pid_result["ip"];
  $net_pid->port_pid = (int)$net_pid_result["port_pid"];
  $net_pid->utime = (int)$net_pid_result["utime"];
  return $net_pid;
}
