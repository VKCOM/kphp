<?php

function do_http_worker() {
  switch($_SERVER["PHP_SELF"]) {
    case "/test_simple_cpu_job": {
      test_simple_cpu_job();
      return;
    }
    case "/test_cpu_job_and_rpc_usage_between": {
      test_cpu_job_and_rpc_usage_between();
      return;
    }
    case "/test_cpu_job_and_mc_usage_between": {
      test_cpu_job_and_mc_usage_between();
      return;
    }
    case "/test_job_script_timeout_error": {
      test_job_script_timeout_error();
      return;
    }
    case "/test_job_errors": {
      test_job_errors();
      return;
    }
  }

  critical_error("unknown test");
}

function send_jobs($context, float $timeout = -1.0): array {
  $ids = [];
  foreach ($context["data"] as $arr) {
    $req = new X2Request;
    $req->tag = (string)$context["tag"];
    $req->master_port = (int)$context["master-port"];
    $req->arr_request = (array)$arr;
    $req->sleep_time_sec = (int)$context["job-sleep-time-sec"];
    $req->error_type = (string)$context["error-type"];
    $ids[] = kphp_job_worker_start($req, $timeout);
  }
  return $ids;
}

function gather_jobs(array $ids): array {
  $result = [];
  foreach($ids as $id) {
    $resp = kphp_job_worker_wait($id);
    if ($resp instanceof X2Response) {
      $result[] = ["data" => $resp->arr_reply, "stats" => $resp->stats];
    } else if ($resp instanceof KphpJobWorkerResponseError) {
      $result[] = ["error" => $resp->getError(), "error_code" => $resp->getErrorCode()];
    }
  }
  return $result;
}

function test_simple_cpu_job() {
  $context = json_decode(file_get_contents('php://input'));
  $ids = send_jobs($context);
  echo json_encode(["jobs-result" => gather_jobs($ids)]);
}

function test_cpu_job_and_rpc_usage_between() {
  $context = json_decode(file_get_contents('php://input'));
  $ids = send_jobs($context);

  $master_port_connection = new_rpc_connection("localhost", $context["master-port"]);
  $stat_id = rpc_tl_query_one($master_port_connection, ['_' => 'engine.stat']);
  $self_stats = rpc_tl_query_result_one($stat_id);

  echo json_encode(["stats" => $self_stats, "jobs-result" => gather_jobs($ids)]);
}

function test_cpu_job_and_mc_usage_between() {
  $context = json_decode(file_get_contents('php://input'));
  $ids = send_jobs($context);

  $mc = new McMemcache();
  $mc->addServer("localhost", (int)$context["master-port"]);
  $self_stats = $mc->get("stats");

  echo json_encode(["stats" => $self_stats, "jobs-result" => gather_jobs($ids)]);
}

function test_job_script_timeout_error() {
  $context = json_decode(file_get_contents('php://input'));
  $ids = send_jobs($context, (float)$context["script-timeout"]);
  echo json_encode(["jobs-result" => gather_jobs($ids)]);
}

function test_job_errors() {
  $context = json_decode(file_get_contents('php://input'));
  $ids = send_jobs($context);
  echo json_encode(["jobs-result" => gather_jobs($ids)]);
}
