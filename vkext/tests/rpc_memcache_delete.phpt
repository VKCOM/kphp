--TEST--
test 'rpc_memcache_delete' function

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
$memcache->add($TM_PREFIX . 'delete_key1', 1, 0, RPC_MEMCACHE_DEFAULT_TTL);
var_dump($memcache->delete($TM_PREFIX . 'delete_key1') === true);
var_dump($memcache->get($TM_PREFIX . 'delete_key1') === false);

$memcache = get_rpc_memcache_connection();
var_dump($memcache->delete($TM_PREFIX . 'delete_key2') === false);


?>

--EXPECT--
bool(true)
bool(true)
bool(true)
