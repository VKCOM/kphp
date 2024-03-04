<?php

/**
 * @param int $to_block
 * @return \tuple(int[], (future<KphpJobWorkerResponse>|false)[])
 */
function suspend_job_workers(int $to_block) {
  $waiting_workers = [];
  $sync_job_ids = [];
  for ($i = 0; $i < $to_block; $i++) {
    $waiting_worker_id = rand() * rand();
    $req = new X2Request;
    $req->arr_request = [$waiting_worker_id];
    $req->tag = 'sync_job';
    $sync_job_ids[] = kphp_job_worker_start($req, -1);
    $waiting_workers[] = $waiting_worker_id;
    while (instance_cache_fetch(SyncJobCommand::class, "sync_job_started_$waiting_worker_id") === null) {}
  }
  return tuple($waiting_workers, $sync_job_ids);
}

function resume_job_workers(array $waiting_workers, array $sync_job_ids) {
  foreach ($waiting_workers as $id) {
    instance_cache_store("finish_sync_job_$id", new SyncJobCommand('finish'));
  }

  foreach (gather_jobs($sync_job_ids) as $res) {
    if (!isset($res['data'])) {
      critical_error('Error in sync job response');
    }
  }
}

function safe_sleep(float $sleep_time_sec) {
  $s = microtime(true);
  do {
    sched_yield_sleep(0.01);
  } while(microtime(true) - $s < $sleep_time_sec);
}

function assert_eq3($actual, $expected, $msg = "") {
    if ($expected !== $actual) {
        critical_error($msg);
    }
}
