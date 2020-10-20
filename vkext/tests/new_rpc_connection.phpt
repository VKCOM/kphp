--TEST--
test 'vk_new_rpc_connection' function

--SKIPIF--
<?php
require_once 'test_helper.inc';
skip_if_no_rpc_service();
?>

--FILE--
<?php
require_once 'test_helper.inc';

$conn1 = get_rpc_connection();
var_dump($conn1 !== false);

$conn2 = @get_rpc_connection("127.0.0.1", 7777);
var_dump($conn2 === false);

?>
--EXPECT--
bool(true)
bool(true)

