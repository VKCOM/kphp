--TEST--
test 'rpc_memcache_replace' function

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
$memcache->add($TM_PREFIX . 'replace_key1', '1', 0, RPC_MEMCACHE_DEFAULT_TTL);
$res = $memcache->replace($TM_PREFIX . 'replace_key1', '2');
var_dump($res === true);
$res = $memcache->get($TM_PREFIX . 'replace_key1');
var_dump($res === '2');

$memcache = get_rpc_memcache_connection();
$res = $memcache->replace($TM_PREFIX . 'replace_key2', '2');
var_dump($res === false);
$res = $memcache->get($TM_PREFIX . 'replace_key2');
var_dump($res === false);

?>

--EXPECT--
bool(true)
bool(true)
bool(true)
bool(true)