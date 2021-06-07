<?php

use ComplexScenario\CollectStatsJobRequest;

require_once 'SharedImmutableMessageScenario/job_worker.php';

function do_job_worker() {
  $req = kphp_job_worker_fetch_request();
  if ($req instanceof X2Request) {
    switch($req->tag) {
      case "x2_with_rpc_request":
        return x2_with_rpc_request($req);
      case "x2_with_mc_request":
        return x2_with_mc_request($req);
      case "x2_with_sleep":
        return x2_with_sleep($req);
      case "x2_with_error":
        return x2_with_error($req);
      case "x2_with_work_after_response":
        return x2_with_work_after_response($req);
      case "sync_job":
        return sync_job($req);
    }
    if ($req->tag !== "") {
      critical_error("Unknown tag " + $req->tag);
    }
    return simple_x2($req);
  } else if ($req instanceof CollectStatsJobRequest) {
    require_once "ComplexScenario/_job_scenario.php";
    run_job_complex_scenario($req);
  } else {
    run_job_shared_immutable_message_scenario($req);
  }
}

function simple_x2(X2Request $x2_req) {
  $x2_resp = new X2Response;
  foreach ($x2_req->arr_request as $value) {
    $x2_resp->arr_reply[] = $value ** 2;
  }
  kphp_job_worker_store_response($x2_resp);
}

function x2_with_rpc_request(X2Request $x2_req) {
  $master_port_connection = new_rpc_connection("localhost", $x2_req->master_port);
  $stat_id = rpc_tl_query_one($master_port_connection, ['_' => 'engine.stat']);
  $self_stats = rpc_tl_query_result_one($stat_id);

  $x2_resp = new X2Response;
  $x2_resp->stats = (array)$self_stats;
  foreach ($x2_req->arr_request as $value) {
    $x2_resp->arr_reply[] = $value ** 2;
  }
  kphp_job_worker_store_response($x2_resp);
}

function x2_with_mc_request(X2Request $x2_req) {
  $mc = new McMemcache();
  $mc->addServer("localhost", $x2_req->master_port);
  $self_stats = $mc->get("stats");

  $x2_resp = new X2Response;
  $x2_resp->stats = [$self_stats];
  foreach ($x2_req->arr_request as $value) {
    $x2_resp->arr_reply[] = $value ** 2;
  }
  kphp_job_worker_store_response($x2_resp);
}

function safe_sleep(int $sleep_time_sec) {
  $s = microtime(true);
  do {
    sched_yield_sleep(1);
  } while(microtime(true) - $s < $sleep_time_sec);
}

function x2_with_sleep(X2Request $x2_req) {
  $x2_resp = new X2Response;
  foreach ($x2_req->arr_request as $value) {
    $x2_resp->arr_reply[] = $value ** 2;
  }
  safe_sleep($x2_req->sleep_time_sec);
  kphp_job_worker_store_response($x2_resp);
}

function stack_overflow($i = 0) {
  stack_overflow($i + 1);

  if ($i % 1000 === 0) {
    fwrite(STDERR, "$i\n");
  }
}

function x2_with_error(X2Request $x2_req) {
  $x2_resp = new X2Response;
  foreach ($x2_req->arr_request as $value) {
    $x2_resp->arr_reply[] = $value ** 2;
  }

  if ($x2_req->error_type === "memory_limit") {
    $arr = [];
    $i = 0;
    while (true) {
      $arr[] = $i++;
    }
  } else if ($x2_req->error_type === "exception") {
    throw new Exception("Test exception");
  } else if ($x2_req->error_type === "stack_overflow") {
    stack_overflow();
  } else if ($x2_req->error_type === "php_assert") {
    critical_error("Test php_assert");
  } else if ($x2_req->error_type === "sigsegv") {
    raise_sigsegv();
  } else if ($x2_req->error_type === "big_response") {
    $x2_resp->arr_reply = make_big_fake_array();
  }

  kphp_job_worker_store_response($x2_resp);
}

function x2_with_work_after_response(X2Request $x2_req) {
  simple_x2($x2_req);
  fprintf(STDERR, "start work after response\n");
  safe_sleep($x2_req->sleep_time_sec);
  fprintf(STDERR, "finish work after response\n");
}

function sync_job(X2Request $req) {
  $id = $req->arr_request[0];
  instance_cache_store("sync_job_started_$id", new SyncJobCommand('started'));

  while (instance_cache_fetch(SyncJobCommand::class, "finish_sync_job_$id") === null) {
  }

  kphp_job_worker_store_response(new X2Response);
}
