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

function tuple(...$args) { return $args;}
#endif

function return_tuple1() {
  sched_yield();
  return tuple(1, 2, "", []);
}

function return_tuple2() {
  sched_yield();
  return tuple(1, 2, [1, 1], [1, ""]);
}

function to_array($x) {
  return [$x[0], $x[1], $x[2], $x[3]];
}

var_dump(to_array(wait_result(fork(return_tuple1()))));
var_dump(to_array(wait_result(fork(return_tuple2()))));

$q = [fork(return_tuple1()), fork(return_tuple2())];
var_dump(to_array(wait_result($q[0])));
var_dump(to_array(wait_result($q[1])));
