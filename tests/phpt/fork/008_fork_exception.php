@ok k2_skip
<?php

require_once 'kphp_tester_include.php';

/**
 * @return Exception
 */
function return_exception() {
  sched_yield();
  return new Exception();
}

/**
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
