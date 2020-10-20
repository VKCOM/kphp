--TEST--
test 'tl_query_result' function

--SKIPIF--
<?php
require_once 'test_helper.inc';
skip_if_no_rpc_service();
?>

--FILE--
<?php
require_once 'test_helper.inc';

$conn = get_rpc_connection();
$q1 = ['engine.pid'];
$qid1 = rpc_tl_query($conn, [$q1]);
var_dump(!empty(rpc_tl_query_result($qid1)[0]['result']));

$conn = get_rpc_connection();
$q4 = ['engine.pid'];
$qid4 = rpc_tl_query($conn, [$q4]);
var_dump(!empty(rpc_tl_query_result($qid4, 2.)[0]['result']));

$conn = get_rpc_connection();
var_dump(empty(rpc_tl_query_result([0])[0]['result']));

?>

--EXPECT--
bool(true)
bool(true)
bool(true)
