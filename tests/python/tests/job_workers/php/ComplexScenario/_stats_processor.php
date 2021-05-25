<?php

use ComplexScenario\NetPid;
use ComplexScenario\Stats;

function mc_get(string $key, int $master_port): string {
  static $mc = null;
  if ($mc === null) {
    $mc = new McMemcache();
    $mc->addServer("localhost", $master_port, true, 1, 10);
  }
  sched_yield();
  $attempts = 3;
  do {
    $result = $mc->get($key);
    if (is_string($result)) {
      return $result;
    }
  } while (--$attempts >= 0);
  critical_error("mc get timeout!");
  return "";
}

function rpc_query(array $request, int $master_port) {
  static $connection = null;
  if ($connection === null) {
    $connection = new_rpc_connection("localhost", $master_port);
  }
  return rpc_tl_query_one($connection, $request);
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

function parse_mc_stats(string $stats_raw, array $allowed_stat_names): Stats {
  $stats = new Stats();
  foreach (explode("\n", $stats_raw) as $stat_line) {
    [$key, $value] = preg_split('#\s+#', $stat_line, 2);
    add_stat_to($stats, $allowed_stat_names, $key, $value);
  }
  $stats->normalize();
  return $stats;
}

function parse_rpc_stats(array $stats_raw, array $allowed_stat_names = []): Stats {
  $stats = new Stats();
  foreach ($stats_raw["result"] as $key => $value) {
    add_stat_to($stats, $allowed_stat_names, $key, $value);
  }
  $stats->normalize();
  return $stats;
}

function get_worker_pids(int $master_port): array {
  $pids_raw = mc_get("workers_pids", $master_port);
  return array_map("intval", explode(",", $pids_raw));
}

function start_net_pid_processing(int $master_port): int {
  return rpc_query(['_' => 'engine.pid'], $master_port);
}

function finish_net_pid_processing(int $processing_id): NetPid {
  $net_pid_result = rpc_tl_query_result_one($processing_id);
  $net_pid_result = $net_pid_result["result"];
  $net_pid = new NetPid;
  $net_pid->ip = (int)$net_pid_result["ip"];
  $net_pid->port_pid = (int)$net_pid_result["port_pid"];
  $net_pid->utime = (int)$net_pid_result["utime"];
  return $net_pid;
}
