@kphp_should_fail k2_skip
/fork of builtin function is forbidden/
<?php

function demo() {
  $engine = new_rpc_connection("127.0.0.1", 5555, 0, 1000000, 1000000);
  $q = ['_' => 'memcache.set', 'key' => '111', 'flags' => 0, 'delay' => 100000, 'value' => '123'];
  $qid = rpc_tl_query_one($engine, $q);
  $f1 = fork(rpc_tl_query_result_one($qid));
  $f2 = fork(rpc_get($qid));
}

demo();
