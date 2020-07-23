@ok
<?php

require_once 'polyfills.php';

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
  $x = wait_result(fork(return_exception()));
  echo "Not catched!\n";
} catch (Exception $e) {
  echo "Catched!\n";
}

try {
  $y = wait_result(fork(throw_exception()));
} catch (Exception $e) {
  echo "Catched!\n";
}
