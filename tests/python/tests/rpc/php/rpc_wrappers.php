<?php

use VK\TL\_common\Types\rpcResponseError;
use VK\TL\_common\Types\rpcResponseHeader;
use VK\TL\_common\Types\rpcResponseOk;
use \VK\TL\_common\Types\rpcInvokeReqExtra;
use \VK\TL\_common\Functions\rpcDestActor;
use \VK\TL\_common\Functions\rpcDestFlags;
use \VK\TL\_common\Functions\rpcDestActorFlags;
use VK\TL\engine\Functions\engine_stat;
use VK\TL\RpcFunction;

if (false) {
    new rpcResponseOk;
    new rpcResponseHeader;
    new rpcResponseError;
}

function test_rpc_no_wrappers_with_actor_id() {
  $actor_id = 1997;
  $conn = new_rpc_connection("localhost", (int)$_GET["master-port"], $actor_id);

  $query_ids = typed_rpc_tl_query($conn, [new engine_stat()]);
  $resp = typed_rpc_tl_query_result($query_ids)[0];

  if ($resp instanceof rpcResponseError) {
    echo json_encode(["error" => $resp->error_code]);
    return;
  }

  critical_error("unreachable");
}

function test_rpc_no_wrappers_with_ignore_answer() {
  $conn = new_rpc_connection("localhost", (int)$_GET["master-port"]);

  $query_ids = typed_rpc_tl_query($conn, [new engine_stat()], -1.0, true);
  $resp = typed_rpc_tl_query_result($query_ids)[0];

  if ($resp instanceof rpcResponseError) {
    echo json_encode(["error" => $resp->error_code]);
    return;
  }

  critical_error("unreachable");
}

function test_rpc_dest_actor_with_actor_id() {
  $actor_id = 1997;
  $conn = new_rpc_connection("localhost", (int)$_GET["master-port"], 0);

  $query_ids = typed_rpc_tl_query($conn, [new rpcDestActor($actor_id, new engine_stat())]);
  $resp = typed_rpc_tl_query_result($query_ids)[0];

  if ($resp instanceof rpcResponseError) {
    echo json_encode(["error" => $resp->error_code]);
    return;
  }

  critical_error("unreachable");
}

function test_rpc_dest_actor_with_ignore_answer() {
  $conn = new_rpc_connection("localhost", (int)$_GET["master-port"], 0);

  $query_ids = typed_rpc_tl_query($conn, [new rpcDestActor(0, new engine_stat())], -1.0, true);
  $resp = typed_rpc_tl_query_result($query_ids)[0];

  if ($resp instanceof rpcResponseError) {
    echo json_encode(["error" => $resp->error_code]);
    return;
  }

  critical_error("unreachable");
}

function test_rpc_dest_flags_with_actor_id() {
  $actor_id = 1997;
  $conn = new_rpc_connection("localhost", (int)$_GET["master-port"], $actor_id);

  $query_ids = typed_rpc_tl_query($conn, [new rpcDestFlags(0, new rpcInvokeReqExtra(), new engine_stat())]);
  $resp = typed_rpc_tl_query_result($query_ids)[0];

  if ($resp instanceof rpcResponseError) {
    echo json_encode(["error" => $resp->error_code]);
    return;
  }

  critical_error("unreachable");
}

function test_rpc_dest_flags_with_ignore_answer() {
  $conn = new_rpc_connection("localhost", (int)$_GET["master-port"], 0);

  $query_ids = typed_rpc_tl_query($conn, [new rpcDestFlags(0, new rpcInvokeReqExtra(), new engine_stat())], -1.0, true);
  $resp = typed_rpc_tl_query_result($query_ids)[0];

  if ($resp instanceof rpcResponseError) {
    echo json_encode(["error" => $resp->error_code]);
    return;
  }

  critical_error("unreachable");
}

function test_rpc_dest_flags_with_ignore_answer_1() {
  $conn = new_rpc_connection("localhost", (int)$_GET["master-port"], 0);

  $query_ids = typed_rpc_tl_query($conn, [new rpcDestFlags(rpcInvokeReqExtra::BIT_NO_RESULT_7, new rpcInvokeReqExtra(), new engine_stat())], -1.0, true);
  $resp = typed_rpc_tl_query_result($query_ids)[0];

  if ($resp instanceof rpcResponseError) {
    echo json_encode(["error" => $resp->error_code]);
    return;
  }

  critical_error("unreachable");
}

function test_rpc_dest_actor_flags_with_actor_id() {
  $actor_id = 1997;
  $conn = new_rpc_connection("localhost", (int)$_GET["master-port"], $actor_id);

  $query_ids = typed_rpc_tl_query($conn, [new rpcDestActorFlags(0, 0, new rpcInvokeReqExtra(), new engine_stat())]);
  $resp = typed_rpc_tl_query_result($query_ids)[0];

  if ($resp instanceof rpcResponseError) {
    echo json_encode(["error" => $resp->error_code]);
    return;
  }

  critical_error("unreachable");
}

function test_rpc_dest_actor_flags_with_ignore_answer() {
  $conn = new_rpc_connection("localhost", (int)$_GET["master-port"], 0);

  $query_ids = typed_rpc_tl_query($conn, [new rpcDestActorFlags(0, 0, new rpcInvokeReqExtra(), new engine_stat())], -1.0, true);
  $resp = typed_rpc_tl_query_result($query_ids)[0];

  if ($resp instanceof rpcResponseError) {
    echo json_encode(["error" => $resp->error_code]);
    return;
  }

  critical_error("unreachable");
}

function test_rpc_dest_actor_flags_with_ignore_answer_1() {
  $conn = new_rpc_connection("localhost", (int)$_GET["master-port"], 0);

  $query_ids = typed_rpc_tl_query($conn, [new rpcDestActorFlags(0, rpcInvokeReqExtra::BIT_NO_RESULT_7, new rpcInvokeReqExtra(), new engine_stat())], -1.0, true);
  $resp = typed_rpc_tl_query_result($query_ids)[0];

  if ($resp instanceof rpcResponseError) {
    echo json_encode(["error" => $resp->error_code]);
    return;
  }

  critical_error("unreachable");
}
