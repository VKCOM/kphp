--TEST--
test 'rpc_tl_result_one' function

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
var_dump(!empty(rpc_tl_query_result_one($qid1)["result"]));
var_dump(empty(rpc_tl_query_result_one($qid1)[0]));

$conn2 = get_rpc_connection();
$q2 = ['engine.pid'];
$qid2 = rpc_tl_query_one($conn2, $q2);
$qid3 = rpc_tl_query_one($conn2, $q2);
$result2 = rpc_tl_query_result_one([$qid2, $qid3]);
var_dump(!empty($result2['__error']));
var_dump(empty($result2['result']));

$conn2 = get_rpc_connection();
$q2 = ['engine.pid'];
$result2 = rpc_tl_query_result_one(null);
var_dump(!empty($result2['__error']));
var_dump(empty($result2['result']));

$conn2 = get_rpc_connection();
$q2 = ['engine.pid'];
$result2 = rpc_tl_query_result_one(0);
var_dump(!empty($result2['__error']));
var_dump(empty($result2['result']));

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
