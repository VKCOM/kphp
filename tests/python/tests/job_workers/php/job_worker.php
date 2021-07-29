<?php

use ComplexScenario\CollectStatsJobRequest;

require_once 'SharedImmutableMessageScenario/job_worker.php';
require_once 'utils.php';

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
      case "redirect_to_job_worker":
        return redirect_to_job_worker($req);
      case "self_lock_job":
        return self_lock_job($req);
      case "x2_no_reply":
        return x2_no_reply($req);
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

function engine_stat_rpc(int $master_port) {
  $master_port_connection = new_rpc_connection("localhost", $master_port);
  $stat_id = rpc_tl_query_one($master_port_connection, ['_' => 'engine.stat']);
  return rpc_tl_query_result_one($stat_id);
}

function simple_x2(X2Request $x2_req) {
  $x2_resp = new X2Response;
  foreach ($x2_req->arr_request as $value) {
    $x2_resp->arr_reply[] = $value ** 2;
  }
  kphp_job_worker_store_response($x2_resp);
}

function x2_with_rpc_request(X2Request $x2_req) {
  $self_stats = engine_stat_rpc($x2_req->master_port);

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

function x2_no_reply(X2Request $x2_req) {
  $sum = 0;
  foreach ($x2_req->arr_request as $item) {
    $sum += $item;
  }
  safe_sleep($x2_req->sleep_time_sec);
  fprintf(STDERR, "Finish no reply job: sum = $sum\n");
}

function sync_job(X2Request $req) {
  $id = $req->arr_request[0];
  instance_cache_store("sync_job_started_$id", new SyncJobCommand('started'));

  while (instance_cache_fetch(SyncJobCommand::class, "finish_sync_job_$id") === null) {
  }

  kphp_job_worker_store_response(new X2Response);
}

function self_lock_job(X2Request $req) {
  [$waiting_workers, $sync_job_ids] = suspend_job_workers(get_job_workers_number() - 1);
  fwrite(STDERR, posix_getpid() . ": Sync jobs started\n");

  /**
   * @var \KphpJobWorkerRequest[]
   */
  $reqs = [];
  foreach ($req->arr_request as $item) {
    $new_req = new X2Request;
    $new_req->arr_request = [$item];
    $reqs[] = $new_req;
  }
  $req_ids = kphp_job_worker_start_multi($reqs, 5);
  engine_stat_rpc($req->master_port); // go to event loop to check we won't grab any of jobs we have just sent

  fwrite(STDERR, posix_getpid() . ": Simple jobs were sent\n");

  $resumable_ids = [];
  foreach ($req_ids as $req_id) {
    if (!$req_id) {
      critical_error("Some simple job is false after start\n");
    } else {
      $resumable_ids[] = $req_id;
    }
  }

  resume_job_workers($waiting_workers, $sync_job_ids);

  $resp = new X2Response;
  /**
   * @var \KphpJobWorkerResponse[]
   */
  $responses = wait_multi($resumable_ids);
  foreach ($responses as $cur_resp) {
    if ($cur_resp instanceof X2Response) {
      $resp->arr_reply[] = $cur_resp->arr_reply[0];
    } else if ($cur_resp instanceof \KphpJobWorkerResponseError) {
      critical_error($cur_resp->getError());
    } else {
      critical_error("Unknown response\n");
    }
  }
  kphp_job_worker_store_response($resp);
}

function redirect_to_job_worker(X2Request $req) {
  $timeout = $req->redirect_chain_len;
  if (--$req->redirect_chain_len == 0) {
    $req->tag = "";
  }
  $res_id = kphp_job_worker_start($req, $timeout);
  $res = wait($res_id);
  kphp_job_worker_store_response($res);
}
