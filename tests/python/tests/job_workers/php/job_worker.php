<?php

function do_job_worker() {
  $req = kphp_job_worker_fetch_request();
  $x2_req = instance_cast($req, X2Request::class);

  switch($x2_req->tag) {
    case "x2_with_rpc_request":
      return x2_with_rpc_request($x2_req);
    case "x2_with_mc_request":
      return x2_with_mc_request($x2_req);
  }
  if ($x2_req->tag !== "") {
    critical_error("Unknown tag " + $x2_req->tag);
  }
  return simple_x2($x2_req);
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
