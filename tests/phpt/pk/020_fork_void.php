@ok
<?php

require_once __DIR__ . "/../dl/polyfill/fork-php-polyfill.php";

function return_void() {
  sched_yield();
  echo "return_void";
}

function throw_exception() {
  sched_yield();
  throw new Exception();
}

wait_result(fork(return_void()));

try {
  wait_result(fork(throw_exception()));
  echo "Not catched!\n";
} catch (Exception $e) {
  echo "Catched!\n";
}
