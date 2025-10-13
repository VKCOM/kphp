--TEST--
test 'rpc_tl_query_result' function

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
$forwardQuery = rpcForward(array(0, 0), $q1);
$qid1 = rpc_tl_query($conn1, array($forwardQuery));
var_dump(!empty(rpc_tl_query_result($qid1)[0]['result']));

?>
--EXPECT--
bool(true)
