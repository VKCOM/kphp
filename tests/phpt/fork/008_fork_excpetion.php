@ok
<?php

require_once 'kphp_tester_include.php';

/**
 * @kphp-infer
 * @return Exception
 */
function return_exception() {
  sched_yield();
  return new Exception();
}

/**
 * @kphp-infer
 * @return int
 */
function throw_exception() {
  sched_yield();
  if (0) return 1;
  throw new Exception();
}

try {
  $x = wait(fork(return_exception()));
  echo "Not catched!\n";
} catch (Exception $e) {
  echo "Catched!\n";
}

try {
  $y = wait(fork(throw_exception()));
} catch (Exception $e) {
  echo "Catched!\n";
}
