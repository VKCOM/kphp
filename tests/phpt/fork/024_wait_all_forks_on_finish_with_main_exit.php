@ok
<?php

function function_return_void(string $msg) {
  echo "start function_return_void($msg)\n";
  sched_yield();
  echo "finish function_return_void($msg)\n";
  return null;
}

function test_kphp() {
  set_wait_all_forks_on_finish();

  fork(function_return_void("111"));
  fork(function_return_void("222"));

  exit(0);
}

#ifndef KPHP

function test_php() {
  echo "start function_return_void(111)\n";
  echo "start function_return_void(222)\n";
}

test_php();
return;
#endif

test_kphp();
