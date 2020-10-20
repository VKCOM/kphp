<?php

function create_const_from_env($name, $default_val, $conversion_fn = null) {
  $env_val = getenv($name, true);
  if ($env_val !== false) {
    $default_val = $env_val;
    if ($conversion_fn !== null) {
      /** @var callable $conversion_fn */
      $default_val = $conversion_fn($default_val);
    }
  }
  define ($name, $default_val);
}

create_const_from_env ("RPC_SERVER_HOST", "localhost");
create_const_from_env ("RPC_SERVER_PORT", 60001, 'intval');
create_const_from_env ("RPC_ACTOR_ID", 0, 'intval');

create_const_from_env ("RPC_MEMCACHE_HOST", "localhost");
create_const_from_env ("RPC_MEMCACHE_PORT", 60001, 'intval');
create_const_from_env ("RPC_MEMCACHE_ACTOR_ID", 0, 'intval');
create_const_from_env ("RPC_MEMCACHE_QUERY_TIMEOUT", 10, 'intval');
create_const_from_env ("RPC_MEMCACHE_CONN_TIMEOUT", 10, 'intval');
create_const_from_env ("RPC_MEMCACHE_RETRY_TIMEOUT", 10, 'intval');
create_const_from_env ("RPC_MEMCACHE_DEFAULT_TTL", 100, 'intval');

define('TL_NET_PID', 1178639567);

ini_set("vkext.use_unix", 1);
ini_set( "tl.conffile", "/projects/engine/objs/bin/combined.tlo");

global $TM_PREFIX;
$TM_PREFIX = microtime();


function get_rpc_connection ($host = RPC_SERVER_HOST, $port = RPC_SERVER_PORT, $actor_id = RPC_ACTOR_ID)
{
	return new_rpc_connection($host, $port, $actor_id);
}

function get_rpc_memcache_connection ($host = RPC_MEMCACHE_HOST,
                                      $port = RPC_MEMCACHE_PORT,
                                      $actor = RPC_MEMCACHE_ACTOR_ID,
                                      $query_timeout = RPC_MEMCACHE_QUERY_TIMEOUT,
                                      $connect_timeout = RPC_MEMCACHE_CONN_TIMEOUT,
                                      $retry_timeout = RPC_MEMCACHE_RETRY_TIMEOUT) {

  $memcache = new RpcMemcache();
  if (!$memcache->rpc_connect($host, $port, $actor, $query_timeout, $connect_timeout, $retry_timeout)) {
    return false;
  }

  return $memcache;
}


function skip_if_no_rpc_service() {
  if (get_rpc_connection() === false) {
    print "skip couldn't connect to rpc";
  }
}

function skip_if_no_rpc_memcache_service() {
  if (get_rpc_memcache_connection() === false) {
    print "skip couldn't connect to memcache rpc";
  }
}

function eq_in_range($expected, $actual, $delta) {
  return (($expected + $delta) >= $actual) && (($expected - $delta) <= $actual);
}

function rpcForward($forward, $query) {
  if ($forward === '*') {
    $query_forward = array('rpcProxy.diagonal', false, $query);
  } else {
    if (is_array($forward)) {
      if (is_int($forward[0])) {
        $flags = (1 << 19);
        $extra = array('int_forward_keys' => $forward);
      } else {
        $flags = (1 << 18);
        $extra = array('string_forward_keys' => $forward);
      }
    } else {
      if (is_int($forward)) {
        $flags = (1 << 21);
        $extra = array('int_forward' => $forward);
      } else {
        $flags = (1 << 20);
        $extra = array('string_forward' => $forward);
      }
    }
    $query_forward = array('rpcDestFlags', false, $flags, $extra, $query);
  }
  return $query_forward;
}
