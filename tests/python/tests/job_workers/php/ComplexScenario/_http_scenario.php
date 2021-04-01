<?php

use \ComplexScenario\CollectStatsJobRequest;
use \ComplexScenario\CollectStatsJobResponse;
use \ComplexScenario\NetPid;
use \ComplexScenario\Stats;
use \ComplexScenario\StatTraits;

require_once "_stats_processor.php";

function collect_stats_with_job(StatTraits $traits, int $master_port) {
  $job_id = kphp_job_worker_start(new CollectStatsJobRequest($master_port, $traits));
  $result = kphp_job_worker_wait($job_id);
  $result_stats = instance_cast($result, CollectStatsJobResponse::class);
  if ($result_stats === null) {
    critical_error("got empty job response");
  }
  return tuple($result_stats->stats, $result_stats->workers_pids, $result_stats->server_net_pid);
}

function collect_stats_locally(StatTraits $traits, int $master_port) {
  $local_processing_id = start_stats_processing($traits, $master_port);
  sched_yield();

  $net_pid_processing_id = start_net_pid_processing($master_port);
  sched_yield();

  $workers_pids = get_worker_pids($master_port);
  $local_stats = finish_stats_processing($traits, $local_processing_id);
  $net_pid = finish_net_pid_processing($net_pid_processing_id);
  return tuple($local_stats, $workers_pids, $net_pid);
}

function compare_job_and_local_stats(StatTraits $traits, int $master_port, string $type) : int {
  $job_wait_id = fork(collect_stats_with_job($traits, $master_port));
  $local_wait_id = fork(collect_stats_locally($traits, $master_port));

  /**
   * @var ?NetPid $job_net_pid
   * @var ?Stats $job_stats
   */
  [$job_stats, $jop_workers_pids, $job_net_pid] = wait($job_wait_id);
  /**
   * @var NetPid $local_net_pid
   * @var Stats $local_stats
   */
  [$local_stats, $local_workers_pids, $local_net_pid] = wait($local_wait_id);

  if ($jop_workers_pids !== $local_workers_pids) {
    critical_error("Worker pids are different!");
  }
  if (!$local_net_pid->equals($job_net_pid)) {
    critical_error("Net pids are different!");
  }
  if (array_keys($job_stats->string_stats) !== array_keys($local_stats->string_stats)) {
    fprintf(STDERR, "\n");
    fprintf(STDERR, json_encode(array_keys($job_stats->string_stats)));
    fprintf(STDERR, "\n");
    fprintf(STDERR, json_encode(array_keys($local_stats->string_stats)));
    fprintf(STDERR, "\n");
    critical_error("Keys of the string $type stats are different!");
  }
  if (array_keys($job_stats->float_stats) !== array_keys($local_stats->float_stats)) {
    fprintf(STDERR, "\n");
    fprintf(STDERR, json_encode(array_keys($job_stats->float_stats)));
    fprintf(STDERR, "\n");
    fprintf(STDERR, json_encode(array_keys($local_stats->float_stats)));
    fprintf(STDERR, "\n");
    critical_error("Keys of the float $type stats are different!");
  }
  if (array_keys($job_stats->int_stats) !== array_keys($local_stats->int_stats)) {
    fprintf(STDERR, "\n");
    fprintf(STDERR, json_encode(array_keys($job_stats->int_stats)));
    fprintf(STDERR, "\n");
    fprintf(STDERR, json_encode(array_keys($local_stats->int_stats)));
    fprintf(STDERR, "\n");
    critical_error("Keys of the int $type stats are different!");
  }

  return sizeof($job_stats->string_stats) + sizeof($job_stats->float_stats) + sizeof($job_stats->int_stats);
}

function run_http_complex_scenario() {
  $context = json_decode(file_get_contents('php://input'));
  $master_port = (int)$context["master-port"];
  $stat_names = array_map("strval", $context["stat-names"]);

  $ids = [
    "mc_stats" => fork(compare_job_and_local_stats(new StatTraits($stat_names, StatTraits::MC_STATS), $master_port, "mc_stats")),
    "mc_stats_full" => fork(compare_job_and_local_stats(new StatTraits($stat_names, StatTraits::MC_STATS_FULL), $master_port, "mc_stats_full")),
    "mc_stats_fast" => fork(compare_job_and_local_stats(new StatTraits($stat_names, StatTraits::MC_STATS_FAST), $master_port, "mc_stats_fast")),
    "rpc_stats" => fork(compare_job_and_local_stats(new StatTraits($stat_names, StatTraits::RPC_STATS), $master_port, "rpc_stats")),
    "rpc_filtered_stats" => fork(compare_job_and_local_stats(new StatTraits($stat_names, StatTraits::RPC_FILTERED_STATS), $master_port, "rpc_filtered_stats")),
  ];

  $result = [];
  foreach ($ids as $type => $id) {
    $result[$type] = wait($id);
  }
  echo json_encode($result);
}
