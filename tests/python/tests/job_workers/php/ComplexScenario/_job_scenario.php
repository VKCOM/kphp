<?php

use \ComplexScenario\CollectStatsJobResponse;
use \ComplexScenario\CollectStatsJobRequest;
use \ComplexScenario\NetPid;
use \ComplexScenario\StatTraits;

function process_stats(StatTraits $traits, int $master_port) {
  $processing_id = start_stats_processing($traits, $master_port);
  return finish_stats_processing($traits, $processing_id);
}

function process_net_pid(int $master_port) : NetPid {
  $net_pid_processing_id = start_net_pid_processing($master_port);
  return finish_net_pid_processing($net_pid_processing_id);
}

function process_worker_pids(int $master_port) : array {
  return get_worker_pids($master_port);
}

function run_job_complex_scenario(KphpJobWorkerRequest $request) {
  if ($request instanceof CollectStatsJobRequest) {
    $stats_fork_id = fork(process_stats($request->traits, $request->master_port));
    $net_pid_fork_id = fork(process_net_pid($request->master_port));
    $workers_pids_fork_id = fork(process_worker_pids($request->master_port));

    $response = new CollectStatsJobResponse();
    $response->stats = wait($stats_fork_id);
    $response->server_net_pid = wait($net_pid_fork_id);
    $response->workers_pids = (array)wait($workers_pids_fork_id);
    kphp_job_worker_store_response($response);
  } else {
    critical_error("Unknown job request");
  }
}
