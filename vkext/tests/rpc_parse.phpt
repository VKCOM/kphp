--TEST--
test 'rpc_parse' function

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
$r1 = rpc_get($qid1);
var_dump(rpc_parse($r1) === true);

$conn2 = get_rpc_connection();
$q2 = ['engine.pid'];
$qid2 = rpc_tl_query_one($conn2, $q2);
$r2 = rpc_get($qid1);
$r2_ref = &$r2;
var_dump(rpc_parse($r2_ref) === true);

?>

--EXPECT--
bool(true)
bool(true)
