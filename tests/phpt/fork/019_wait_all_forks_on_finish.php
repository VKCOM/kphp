@ok
<?php

function function_return_void(string $msg) {
  echo "start function_return_void($msg)\n";
  sched_yield();
  echo "finish function_return_void($msg)\n";
  return null;
}

function function_return_string(string $msg) {
  echo "start function_return_string($msg)\n";
  sched_yield();
  echo "finish function_return_string($msg)\n";
  return "done $msg";
}

function function_with_wait(string $msg) {
  echo "start function_with_wait($msg)\n";
  $x = fork(function_return_void($msg));
  echo "work function_with_wait($msg)\n";
  wait($x);
  echo "finish function_with_wait($msg)\n";
  return null;
}

function function_with_exception1(string $msg) {
  echo "start function_with_exception1($msg)\n";
  sched_yield();
  throw new Exception("Exception $msg");
  echo "finish function_with_exception1($msg)\n";
  return null;
}

function function_with_exception2(string $msg) {
  echo "start function_with_exception2($msg)\n";
  throw new Exception("Exception $msg");
  sched_yield();
  echo "finish function_with_exception2($msg)\n";
  return null;
}

function test_kphp() {
  fork(function_return_void("111"));
  fork(function_return_void("222"));

  fork(function_return_string("333"));
  fork(function_return_string("444"));

  fork(function_with_wait("555"));
  fork(function_with_wait("666"));

  fork(function_with_exception1("777"));
  fork(function_with_exception2("888"));

  set_wait_all_forks_on_finish();
}

#ifndef KPHP

function test_php() {
  echo "start function_return_void(111)\n";
  echo "start function_return_void(222)\n";

  echo "start function_return_string(333)\n";
  echo "start function_return_string(444)\n";

  echo "start function_with_wait(555)\n";
  echo "start function_return_void(555)\n";
  echo "work function_with_wait(555)\n";

  echo "start function_with_wait(666)\n";
  echo "start function_return_void(666)\n";
  echo "work function_with_wait(666)\n";

  echo "start function_with_exception1(777)\n";
  echo "start function_with_exception2(888)\n";

  echo "finish function_return_void(111)\n";
  echo "finish function_return_void(222)\n";

  echo "finish function_return_string(333)\n";
  echo "finish function_return_string(444)\n";

  echo "finish function_return_void(555)\n";
  echo "finish function_with_wait(555)\n";

  echo "finish function_return_void(666)\n";
  echo "finish function_with_wait(666)\n";
}

test_php();
return;
#endif

test_kphp();
