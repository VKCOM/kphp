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
  }

  critical_error("unknown test");
}

function send_jobs($context): array {
  $ids = [];
  foreach ($context["data"] as $arr) {
    $req = new X2Request;
    $req->tag = (string)$context["tag"];
    $req->master_port = (int)$context["master-port"];
    $req->arr_request = (array)$arr;
    $ids[] = kphp_job_worker_start($req);
  }
  return $ids;
}

function gather_jobs(array $ids): array {
  $result = [];
  foreach($ids as $id) {
    $resp = kphp_job_worker_wait($id);
    $x2_resp = instance_cast($resp, X2Response::class);
    $result[] = ["data" => $x2_resp->arr_reply, "stats" => $x2_resp->stats];
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
