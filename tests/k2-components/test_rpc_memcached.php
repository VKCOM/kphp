<?php

function do_untyped_rpc() {
  $ids = rpc_send_requests("mc_main", [["_" => "memcache.get", "key" => "xxx"]], -1, false, null, false);
  $response = rpc_fetch_responses($ids)[0];
  $result = $response["result"];
  if ($result["_"] !== "memcache.not_found") {
    warning("memcache.not_found expected");
    return false;
  }

  $ids = rpc_send_requests("mc_main", [["_" => "memcache.set", "key" => "foo", "flags" => 0, "delay" => 0, "value" => "bar"]], -1, false, null, false);
  $response = rpc_fetch_responses($ids)[0];
  $result = $response["result"];
  if ($result !== true) {
    warning("true expected");
    return false;
  }

  $ids = rpc_send_requests("mc_main", [["_" => "memcache.get", "key" => "foo"]], -1, false, null, false);
  $response = rpc_fetch_responses($ids)[0];
  $result = $response["result"];
  if ($result["_"] !== "memcache.strvalue" || $result["value"] !== "bar") {
    warning("\"bar\" expected");
    return false;
  }

  return true;
}

$query = component_server_accept_query();
if (do_untyped_rpc()) {
  component_server_send_response($query, "OK");
} else {
  component_server_send_response($query, "FAIL");
}
