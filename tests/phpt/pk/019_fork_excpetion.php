<?php

#ifndef KittenPHP
$fork_results = [];

function sched_yield() {}

function fork($x) {
  global $fork_results;
  static $id = 1;
  $fork_results[$id] = $x;
  return $id++;
}

function wait_result($x) {
  global $fork_results;
  if (!isset($fork_results[$x])) {
    return false;
  }
  $res = $fork_results[$x];
  unset($fork_results[$x]);
  return $res;
}

#endif

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
