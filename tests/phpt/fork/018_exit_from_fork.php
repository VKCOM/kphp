@ok
<?php

function function_return_void(string $msg) {
  echo "start function_return_void($msg)\n";
  sched_yield();
  echo "finish function_return_void($msg)\n";
  return null;
}

function call_exit() {
  exit(0);
}

function function_no_return(string $msg) {
  echo "start function_no_return($msg)\n";
  sched_yield();
  call_exit();
  echo "finish function_no_return($msg)\n";
  return 1;
}

function test_kphp() {
  set_wait_all_forks_on_finish();

  fork(function_return_void("111"));
  fork(function_return_void("222"));

  fork(function_no_return("333"));
  fork(function_no_return("444"));
}

#ifndef KPHP

function test_php() {
  echo "start function_return_void(111)\n";
  echo "start function_return_void(222)\n";

  echo "start function_no_return(333)\n";
  echo "start function_no_return(444)\n";

  echo "finish function_return_void(111)\n";
  echo "finish function_return_void(222)\n";
}

test_php();
return;
#endif

test_kphp();
