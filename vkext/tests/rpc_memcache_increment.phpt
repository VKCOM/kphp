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
$memcache->add($TM_PREFIX . 'increment_key1', 1, 0, RPC_MEMCACHE_DEFAULT_TTL);
$res = $memcache->increment($TM_PREFIX . 'increment_key1', 1);
var_dump($res === 2);
$res = $memcache->get($TM_PREFIX . 'increment_key1');
var_dump($res === '2');

$memcache = get_rpc_memcache_connection();
$memcache->add($TM_PREFIX . 'increment_key2', 0, 0, RPC_MEMCACHE_DEFAULT_TTL);
$res = $memcache->increment($TM_PREFIX . 'increment_key2', 0);
var_dump($res === 0);
$res = $memcache->get($TM_PREFIX . 'increment_key2');
var_dump($res === '0');

$memcache = get_rpc_memcache_connection();
$memcache->add($TM_PREFIX . 'increment_key3', 0, 0, RPC_MEMCACHE_DEFAULT_TTL);
$res = $memcache->increment($TM_PREFIX . 'increment_key3', 2);
var_dump($res === 2);
$res = $memcache->get($TM_PREFIX . 'increment_key3');
var_dump($res === '2');

$memcache = get_rpc_memcache_connection();
$memcache->add($TM_PREFIX . 'increment_key4', 3, 0, RPC_MEMCACHE_DEFAULT_TTL);
$res = $memcache->increment($TM_PREFIX . 'increment_key4', -2);
var_dump($res === 1);
$res = $memcache->get($TM_PREFIX . 'increment_key4');
var_dump($res === '1');

$memcache = get_rpc_memcache_connection();
$res = $memcache->increment($TM_PREFIX . 'increment_nonexistent', 0);
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
