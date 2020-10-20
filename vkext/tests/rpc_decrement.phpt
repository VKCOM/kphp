--TEST--
test 'rpc_memcache_increment' function

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
$memcache->add($TM_PREFIX . 'decrement_key1', 1, 0, RPC_MEMCACHE_DEFAULT_TTL);
$res = $memcache->decrement($TM_PREFIX . 'decrement_key1', 1);
var_dump($res === 0);
$res = $memcache->get($TM_PREFIX . 'decrement_key1');
var_dump($res === '0');

$memcache = get_rpc_memcache_connection();
$memcache->add($TM_PREFIX . 'decrement_key2', 2, 0, RPC_MEMCACHE_DEFAULT_TTL);
$res = $memcache->decrement($TM_PREFIX . 'decrement_key2', 2);
var_dump($res === 0);
$res = $memcache->get($TM_PREFIX . 'decrement_key2');
var_dump($res === '0');


$memcache = get_rpc_memcache_connection();
$memcache->add($TM_PREFIX . 'decrement_key3', 1, 0, RPC_MEMCACHE_DEFAULT_TTL);
$res = $memcache->decrement($TM_PREFIX . 'decrement_key3', -1);
var_dump($res === 2);
$res = $memcache->get($TM_PREFIX . 'decrement_key3');
var_dump($res === '2');


$memcache = get_rpc_memcache_connection();
$memcache->add($TM_PREFIX . 'decrement_key4', 1, 0, RPC_MEMCACHE_DEFAULT_TTL);
$res = $memcache->decrement($TM_PREFIX . 'decrement_key4', 0);
var_dump($res === 1);
$res = $memcache->get($TM_PREFIX . 'decrement_key4');
var_dump($res === '1');

$memcache = get_rpc_memcache_connection();
$res = $memcache->decrement($TM_PREFIX . 'decrement_nonexistent', 0);
var_dump($res === false);

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
