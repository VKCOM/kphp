--TEST--
test 'rpc_get_and_parse' function

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
var_dump(rpc_get_and_parse($qid1) === true);

$conn2 = get_rpc_connection();
$q2 = ['engine.pid'];
$qid2 = rpc_tl_query_one($conn2, $q2);
var_dump(rpc_get_and_parse(0) === false);

$conn3 = get_rpc_connection();
$q3 = ['engine.pid'];
$qid3 = rpc_tl_query_one($conn3, $q2);
var_dump(rpc_get_and_parse($qid3, 1.) === true);

?>
--EXPECT--
bool(true)
bool(true)
bool(true)
