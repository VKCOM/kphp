--TEST--
test 'rpc_queue' functions

--SKIPIF--
<?php
require_once 'test_helper.inc';
skip_if_no_rpc_service();
?>

--FILE--
<?php
require_once 'test_helper.inc';

$conn1 = get_rpc_connection();
$q1 = array(array('engine.pid'), array('engine.pid'));
$qids = rpc_tl_query($conn1, $q1);
$que = rpc_queue_create($qids);
var_dump($que !== 0);
var_dump(rpc_queue_empty($que) === false);
while (!rpc_queue_empty($que)) {
  $qid = rpc_queue_next($que);
  var_dump(!empty(rpc_get($qid)));
}

$conn2 = get_rpc_connection();
$q2 = array(array('engine.pid'), array('engine.pid'));
$qids = rpc_tl_query($conn2, $q2);
$que = rpc_queue_create($qids);
$q2 = array('engine.pid');
$qid2 = rpc_tl_query_one($conn1, $q2);
rpc_queue_push($que, $qid2);
while (!rpc_queue_empty($que)) {
  $qid = rpc_queue_next($que);
  var_dump(!empty(rpc_get($qid)));
}

$conn3 = get_rpc_connection();
$que = rpc_queue_create(-1);
var_dump(rpc_queue_empty($que) === true);

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
