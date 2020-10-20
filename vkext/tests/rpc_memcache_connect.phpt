--TEST--
test 'rpc_memcache_connect' function

--FILE--
<?php
require_once 'test_helper.inc';

$conn1 = get_rpc_memcache_connection();
var_dump($conn1 !== false);

?>
--EXPECT--
bool(true)

