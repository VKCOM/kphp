--TEST--
test 'rpc_get' function

--SKIPIF--
<?php
require_once 'test_helper.inc';
skip_if_no_rpc_service();
?>

--FILE--
<?php
require_once 'test_helper.inc';

$conn1 = get_rpc_connection();
$q1 = ['engine.pid'];
$qid1 = rpc_tl_query_one($conn1, $q1);
var_dump(!empty(rpc_get($qid1)));

$conn2 = get_rpc_connection();
$q2 = ['engine.pid'];
$qid2 = rpc_tl_query_one($conn2, $q2);
var_dump(empty(rpc_get(0)));

$conn3 = get_rpc_connection();
$q3 = ['engine.pid'];
$qid3 = rpc_tl_query_one($conn3, $q2);
var_dump(!empty(rpc_get($qid3, 1.)));

?>
--EXPECT--
bool(true)
bool(true)
bool(true)
