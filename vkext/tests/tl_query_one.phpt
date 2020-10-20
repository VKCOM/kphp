--TEST--
test 'rpc_tl_query_one' function

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
var_dump(($qid1 !== false) && ($qid1 !== -1));
var_dump(empty(rpc_get_last_send_error()));

$conn2 = get_rpc_connection();
$q2 = [['engine.pid']];
$qid2 = rpc_tl_query_one($conn2, $q2);
var_dump($qid2 === 0);
var_dump(empty(!rpc_get_last_send_error()));

$conn3 = get_rpc_connection();
$qid3 = rpc_tl_query_one($conn3, null);
var_dump($qid3 === 0);
var_dump(empty(!rpc_get_last_send_error()));

$conn4 = get_rpc_connection();
$q = ['engine.pid'];
$q4 = &$q;
$qid4 = rpc_tl_query_one($conn4, $q4);
var_dump(($qid4 !== false) && ($qid4 !== -1) && ($qid4 !== 0));
var_dump(empty(rpc_get_last_send_error()));


$conn5 = get_rpc_connection();
$q5 = ['engine.pid'];
$qid5 = rpc_tl_query_one($conn5, $q5, 0.);
var_dump(($qid5 !== false) || (($qid5 !== -1) || ($qid5 !== 0)));
var_dump(empty(rpc_get_last_send_error()));

$conn6 = get_rpc_connection();
$q6 = ['engine.pid'];
$qid6 = rpc_tl_query_one($conn6, $q6, 1.);
var_dump(($qid6 !== false) && ($qid6 !== -1) && ($qid6 !== 0));
var_dump(empty(rpc_get_last_send_error()));

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
