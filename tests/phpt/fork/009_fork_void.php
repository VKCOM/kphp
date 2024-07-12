@kphp_should_fail k2_skip
/forking void functions is forbidden, return null at least/
<?php

require_once 'kphp_tester_include.php';

function return_void() {
  sched_yield();
  echo "return_void";
}

function throw_exception() {
  sched_yield();
  throw new Exception();
}

wait(fork(return_void()));

try {
  wait(fork(throw_exception()));
  echo "Not catched!\n";
} catch (Exception $e) {
  echo "Catched!\n";
}
