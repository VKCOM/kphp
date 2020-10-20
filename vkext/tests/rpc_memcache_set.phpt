--TEST--
test 'rpc_memcache_set' function

--SKIPIF--
<?php
require_once 'test_helper.inc';
skip_if_no_rpc_memcache_service();
?>

--FILE--
<?php
require_once 'test_helper.inc';
global $TM_PREFIX;

$memcache = get_rpc_memcache_connection();
var_dump($memcache->set($TM_PREFIX . 'set1', '1', 0, RPC_MEMCACHE_DEFAULT_TTL));

$memcache = get_rpc_memcache_connection();
var_dump($memcache->set($TM_PREFIX . 'set2', 2, 0, RPC_MEMCACHE_DEFAULT_TTL));

$memcache = get_rpc_memcache_connection();
var_dump($memcache->set($TM_PREFIX . 'set1.1', 1.1, 0, RPC_MEMCACHE_DEFAULT_TTL));

$memcache = get_rpc_memcache_connection();
var_dump($memcache->set($TM_PREFIX . 'set0', 0, 0, RPC_MEMCACHE_DEFAULT_TTL));

$memcache = get_rpc_memcache_connection();
var_dump($memcache->set($TM_PREFIX . 'set_empty', '', 0, RPC_MEMCACHE_DEFAULT_TTL));

$memcache = get_rpc_memcache_connection();
var_dump($memcache->set($TM_PREFIX . 'set_false', false, 0, RPC_MEMCACHE_DEFAULT_TTL));

$memcache = get_rpc_memcache_connection();
var_dump($memcache->set($TM_PREFIX . 'set_null', null, 0, RPC_MEMCACHE_DEFAULT_TTL));

$memcache = get_rpc_memcache_connection();
var_dump($memcache->set($TM_PREFIX . 'set_override', 1, 0, RPC_MEMCACHE_DEFAULT_TTL));
var_dump($memcache->set($TM_PREFIX . 'set_override', 2, 0, RPC_MEMCACHE_DEFAULT_TTL));

?>

--EXPECT--
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)

