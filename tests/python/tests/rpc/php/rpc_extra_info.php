<?php

use VK\TL\_common\Types\rpcResponseError;
use VK\TL\_common\Types\rpcResponseHeader;
use VK\TL\_common\Types\rpcResponseOk;
use VK\TL\engine\Functions\engine_version;
use VK\TL\engine\Functions\engine_pid;
use VK\TL\engine\Functions\engine_stat;
use VK\TL\engine\Functions\engine_sleep;
use VK\TL\RpcFunction;

if (false) {
    new rpcResponseOk;
    new rpcResponseHeader;
    new rpcResponseError;
}

function test_kphp_untyped_rpc_extra_info() {
  $queries = [['_' => "engine.stat"], ['_' => "engine.pid"], ['_' => "engine.version"], ["_" => "engine.sleep", "time_ms" => (int)(200)]];
  $conn = new_rpc_connection("localhost", (int)$_GET["master-port"]);
  $requests_extra_info = new \KphpRpcRequestsExtraInfo;

  $query_ids = rpc_tl_query($conn, $queries, -1.0, false, $requests_extra_info, true);

  $res = rpc_tl_query_result($query_ids);

  $responses_extra_info = [];
  foreach ($query_ids as $q_id) {
    $extra_info = extract_kphp_rpc_response_extra_info($q_id);
    if (is_null($extra_info)) {
        critical_error("got null rpc response extra info after processing an rpc response!");
    }
    array_push($responses_extra_info, $extra_info);
  }

  echo json_encode([
    "result" => $res["result"],
    "requests_extra_info" => array_map(fn($req_tup): array => [$req_tup[0]], $requests_extra_info->get()),
    "responses_extra_info" => array_map(fn($resp_tup): array => [$resp_tup[0], $resp_tup[1]], $responses_extra_info),
  ]);
}

function test_kphp_typed_rpc_extra_info() {
  $queries = [new engine_stat(), new engine_pid(), new engine_version(), new engine_sleep(200)];
  $conn = new_rpc_connection("localhost", (int)$_GET["master-port"]);
  $requests_extra_info = new \KphpRpcRequestsExtraInfo;

  $query_ids = typed_rpc_tl_query($conn, $queries, -1.0, false, $requests_extra_info, true);

  $res = typed_rpc_tl_query_result($query_ids);

  $responses_extra_info = [];
  foreach ($query_ids as $q_id) {
    $extra_info = extract_kphp_rpc_response_extra_info($q_id);
    if (is_null($extra_info)) {
        critical_error("got null rpc response extra info after processing an rpc response!");
    }
    array_push($responses_extra_info, $extra_info);
  }

  foreach ($res as $resp) {
    if ($resp->isError()) {
      critical_error("bad rpc response");
    }
  }

  echo json_encode([
    "requests_extra_info" => array_map(fn($req_tup): array => [$req_tup[0]], $requests_extra_info->get()),
    "responses_extra_info" => array_map(fn($resp_tup): array => [$resp_tup[0], $resp_tup[1]], $responses_extra_info),
  ]);
}
