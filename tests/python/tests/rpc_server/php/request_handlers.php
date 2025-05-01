<?php

use \VK\TL\kphp\Functions\kphp_pingRpcCluster;
use \VK\TL\kphp\Functions\kphp_pingRpcCluster_result;

define('ERROR_RESPONSE', -1);
define('OK_RESPONSE', 200);
define('EXPECTED_ACTOR_ID', 11);
define('BINLOG_POS', 22);

/**
 * @param $request kphp_pingRpcCluster
 */
function handle_kphpPingRpcCluster($request) {
  return kphp_pingRpcCluster::createRpcServerResponse(OK_RESPONSE);
}

function handle_rpcDestActor() {
  if ($_SERVER['RPC_ACTOR_ID'] != EXPECTED_ACTOR_ID) {
    return kphp_pingRpcCluster::createRpcServerResponse(ERROR_RESPONSE);
  }
  return kphp_pingRpcCluster::createRpcServerResponse(OK_RESPONSE);
}

function handle_rpcDestFlags() {
  if (!($_SERVER['RPC_EXTRA_FLAGS'] & 0x1)) {
    return kphp_pingRpcCluster::createRpcServerResponse(ERROR_RESPONSE);
  }
  return kphp_pingRpcCluster::createRpcServerResponse(BINLOG_POS);
}

function handle_rpcDestActorFlags() {
  if ($_SERVER['RPC_ACTOR_ID'] != EXPECTED_ACTOR_ID || !($_SERVER['RPC_EXTRA_FLAGS'] & 0x1)) {
    return kphp_pingRpcCluster::createRpcServerResponse(ERROR_RESPONSE);
  }
  return kphp_pingRpcCluster::createRpcServerResponse(BINLOG_POS);
}

/**
 * @return ?kphp_pingRpcCluster_result
 */
function handle_request($request) {
  if (isset($_SERVER['RPC_ACTOR_ID']) && isset($_SERVER['RPC_EXTRA_FLAGS'])) {
    return handle_rpcDestActorFlags();
  } else if (isset($_SERVER['RPC_ACTOR_ID'])) {
    return handle_rpcDestActor();
  } else if (isset($_SERVER['RPC_EXTRA_FLAGS'])) {
    return handle_rpcDestFlags();
  } else if ($request instanceof kphp_pingRpcCluster) {
    return handle_kphpPingRpcCluster($request);
  } else {
    return null;
  }
}

