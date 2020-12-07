@ok
<?php

require_once 'kphp_tester_include.php';

class MyException1 extends Exception {}
class MyException2 extends Exception {}

/**
 * @return Exception
 */
function return_exception() {
  sched_yield();
  return new MyException1();
}

/**
 * @return int
 */
function throw_exception() {
  sched_yield();
  if (0) return 1;
  throw new MyException1();
}

try {
  try {
    $x = wait(fork(return_exception()));
    var_dump(__LINE__);
  } catch (MyException2 $e) {
    var_dump([__LINE__ => 'catch2']);
  }
} catch (MyException1 $e2) {
  var_dump([__LINE__ => [get_class($e2), $e2->getLine()]]);
}

try {
  try {
    $y = wait(fork(throw_exception()));
    var_dump(__LINE__);
  } catch (MyException2 $e) {
    var_dump([__LINE__ => 'catch2']);
  }
} catch (MyException1 $e2) {
  var_dump([__LINE__ => [get_class($e2), $e2->getLine()]]);
}
