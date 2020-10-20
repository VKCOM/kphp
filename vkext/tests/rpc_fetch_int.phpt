--TEST--
test 'fetch_int' function

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
$res1 = rpc_tl_query_result_one($qid1);

$conn2 = get_rpc_connection();
$qid2 = rpc_tl_query_one($conn2, $q1);
rpc_parse(rpc_get($qid2));
var_dump(TL_NET_PID === fetch_int());
var_dump($res1['result']['ip'] === fetch_int());
var_dump($res1['result']['port_pid'] === fetch_int());

?>
--EXPECT--
bool(true)
bool(true)
bool(true)
