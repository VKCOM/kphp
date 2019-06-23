@ok
<?php

require_once __DIR__ . "/../dl/polyfill/fork-php-polyfill.php";

function return_exception() {
  sched_yield();
  return new Exception();
}

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
