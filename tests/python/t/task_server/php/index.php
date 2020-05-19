<?php

$magic = fetch_int();
// fwrite(STDERR,  "~~~~ magic = " . $magic . "\n");
$fm = fetch_int();
if ($fm & (1 << 0)) {
  $timeout = fetch_int();
//   fwrite(STDERR,  "~~~~ timeout = " . $timeout . "\n");
}
$n = fetch_int();
// fwrite(STDERR,  "~~~~ n = " . $n . "\n");
$type_name = fetch_string();
fwrite(STDERR,  "task type ".$type_name."\n");
$queue_id_len = fetch_int();
for ($i = 0; $i < $queue_id_len; $i++) {
	fetch_int();
}
$fields_mask = fetch_int();
$flags = fetch_int();
$tag_len = fetch_int();
for ($i = 0; $i < $tag_len; $i++) {
	fetch_int();
}
$data = fetch_string();
$id = fetch_long();
dbg_echo($type_name." ".$id."\n");

$engine = new_rpc_connection("localhost", (int)ini_get("task_engine_port"));
$query = array("_" => "engine.stat");
$result = rpc_tl_query_result(rpc_tl_query ($engine, array($query)));

rpc_clean();
store_int(0x573801e4); // TL_TASKS_RESPONSE_TASK_DONE
store_string($type_name);
store_long($id);
store_finish();
