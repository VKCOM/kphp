--TEST--
test 'rpc_memcache_get' function

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
$memcache->set($TM_PREFIX . 'test_key1', '1', 0, RPC_MEMCACHE_DEFAULT_TTL);
$res = $memcache->get($TM_PREFIX . 'test_key1');
var_dump($res === '1');

$memcache = get_rpc_memcache_connection();
$memcache->set($TM_PREFIX . 'test_key2', 1, 0, RPC_MEMCACHE_DEFAULT_TTL);
$res = $memcache->get($TM_PREFIX . 'test_key2');
var_dump($res === '1');

$memcache = get_rpc_memcache_connection();
$memcache->set($TM_PREFIX . 'test_key3', 1.1, 0, RPC_MEMCACHE_DEFAULT_TTL);
$res = $memcache->get($TM_PREFIX . 'test_key3');
var_dump($res === '1.1');

$memcache = get_rpc_memcache_connection();
$memcache->set($TM_PREFIX . 'get_empty', '', 0, RPC_MEMCACHE_DEFAULT_TTL);
$res = $memcache->get($TM_PREFIX . 'get_empty');
var_dump($res === '');

$memcache = get_rpc_memcache_connection();
$memcache->set($TM_PREFIX . 'get_null', null, 0, RPC_MEMCACHE_DEFAULT_TTL);
$res = $memcache->get($TM_PREFIX . 'get_null');
var_dump($res === null);

$memcache = get_rpc_memcache_connection();
$memcache->set($TM_PREFIX . 'get_empty', 0, 0, RPC_MEMCACHE_DEFAULT_TTL);
$res = $memcache->get($TM_PREFIX . 'get_empty');
var_dump($res === '0');

$memcache = get_rpc_memcache_connection();
$res = $memcache->get($TM_PREFIX . 'test_key4');
var_dump($res === false);

$memcache = get_rpc_memcache_connection();
$memcache->set($TM_PREFIX . 'test_key4', [1,2,3], 0, RPC_MEMCACHE_DEFAULT_TTL);
$res = $memcache->get($TM_PREFIX . 'test_key4');
var_dump(is_array($res));
var_dump(!empty($res[0]) && ($res[0] === 1));
var_dump(!empty($res[1]) && ($res[1] === 2));
var_dump(!empty($res[2]) && ($res[2] === 3));

$memcache = get_rpc_memcache_connection();
$multi_key1 = $TM_PREFIX . 'multi_key1';
$multi_key2 = $TM_PREFIX . 'multi_key2';
$memcache->set($multi_key1, 1, 0, RPC_MEMCACHE_DEFAULT_TTL);
$memcache->set($multi_key2, 2, 0, RPC_MEMCACHE_DEFAULT_TTL);
$res = $memcache->get([$multi_key1, $multi_key2]);
var_dump(is_array($res));
var_dump(!empty($res[$multi_key1]) && ($res[$multi_key1]==='1'));
var_dump(!empty($res[$multi_key2]) && ($res[$multi_key2]==='2'));

$memcache = get_rpc_memcache_connection();
$memcache->set($TM_PREFIX . 'get_override', 1, 0, RPC_MEMCACHE_DEFAULT_TTL);
$res = $memcache->get($TM_PREFIX . 'get_override');
var_dump($res == '1');
$memcache->set($TM_PREFIX . 'get_override', 2, 0, RPC_MEMCACHE_DEFAULT_TTL);
$res = $memcache->get($TM_PREFIX . 'get_override');
var_dump($res == '2');

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
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
