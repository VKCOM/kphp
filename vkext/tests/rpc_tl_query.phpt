--TEST--
test 'rpc_tl_query' function

--SKIPIF--
<?php
require_once 'test_helper.inc';
skip_if_no_rpc_service();
?>

--FILE--
<?php
require_once 'test_helper.inc';

$conn = get_rpc_connection();
$q = ['engine.pid'];
$qid1 = rpc_tl_query($conn, [$q]);
var_dump(!!$qid1);
var_dump(empty(rpc_get_last_send_error()));


$conn2 = get_rpc_connection();
$q = ['engine.pid'];
$qid2 = rpc_tl_query($conn, [$q], 0);
var_dump($qid2 !== false);
var_dump(empty(rpc_get_last_send_error()));


$conn3 = get_rpc_connection();
$q = ['engine.pid'];
$qid3 = rpc_tl_query($conn, [$q], 0, true);
var_dump($qid3[0] === -1);
var_dump(empty(rpc_get_last_send_error()));

$conn4 = get_rpc_connection();
$q = ['engine.pid'];
$qid4 = rpc_tl_query($conn, [$q], 0, false);
var_dump(($qid4[0] !== -1) && ($qid4[0] !== false));
var_dump(empty(rpc_get_last_send_error()));


$qid4 = rpc_tl_query(null, [$q]);
var_dump($qid4 === false);
var_dump(!empty(rpc_get_last_send_error()));


$qid5 = rpc_tl_query(null, [['engine.incorrect_q']]);
var_dump($qid5 === false);
var_dump(!empty(rpc_get_last_send_error()));

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
