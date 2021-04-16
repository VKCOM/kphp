<?php

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
    }
    if ($req->tag !== "") {
      critical_error("Unknown tag " + $req->tag);
    }
    return simple_x2($req);
  } else {
    require_once "ComplexScenario/_job_scenario.php";
    run_job_complex_scenario($req);
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

function x2_with_sleep(X2Request $x2_req) {
  $x2_resp = new X2Response;
  foreach ($x2_req->arr_request as $value) {
    $x2_resp->arr_reply[] = $value ** 2;
  }
  $s = microtime(true);
  do {
    sched_yield_sleep(1);
  } while(microtime(true) - $s < $x2_req->sleep_time_sec);

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
  }if ($x2_req->error_type === "sigsegv") {
    raise_sigsegv();
  }

  kphp_job_worker_store_response($x2_resp);
}
