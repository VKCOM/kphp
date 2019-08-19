<?php

#ifndef KittenPHP // good
require_once "../phpt/polyfills.php";

function ClassAutoLoader($class)
{
  $home_dir = getenv("HOME");
  require_once $home_dir . '/engine/PHP/tests/dev/' . str_replace('\\', '/', $class . '.php');
}

spl_autoload_register('ClassAutoLoader', true, true);

function set_tl_mode($x)
{
  global $tl_mode;
  $tl_mode = $x;
}
#endif

const SAFE_NEW_TL_MODE = 0;
const FULL_OLD_TL_MODE = 1;
const FULL_NEW_TL_MOD = 2;

const MEMCACHE_PORT = 5555;
const GRAPH_PORT = 6666;
const TEXT_PORT = 8888;

function send_and_vardump($q, $port, $actor_id = 0)
{
  $engine = new_rpc_connection("127.0.0.1", $port, $actor_id, 1000000, 1000000);
  $qid = rpc_tl_query_one($engine, $q);
  echo "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n";
  $res = rpc_tl_query_result_one($qid);
  var_dump($res);
}

/**
 * @param int $q
 * @param int $port
 * @param int $actor_id
 * @return VK\TL\RpcResponse
 */
function rpc_query_sync($q, $port, $actor_id = 0) {
  $conn = new_rpc_connection("127.0.0.1", $port, $actor_id, 1000000, 1000000);
  $set_id = typed_rpc_tl_query_one($conn, $q);
  $res = typed_rpc_tl_query_result_one($set_id);
  return $res;
}

new \VK\TL\Types\rpcResponseOk;
new \VK\TL\Types\rpcResponseHeader;
new \VK\TL\Types\rpcResponseError;
