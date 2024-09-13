@ok k2_skip
<?php

require_once 'kphp_tester_include.php';

/**
 * @return null
 */
function return_null() {
  sched_yield();
  return null;
}

/**
 * @return true
 */
function return_true() {
  sched_yield();
  return true;
}

/**
 * @return false
 */
function return_false() {
  sched_yield();
  return false;
}

/**
 * @param $x int
 * @return true|false|null
 */
function return_true_false_null($x) {
  sched_yield();
  if ($x == 1) {
    return true;
  }
  if ($x == 2) {
    return false;
  }
  return null;
}

/**
 * @param $x int
 * @return false|null|string[]
 */
function return_false_null_array($x) {
  sched_yield();
  if ($x == 1) {
    return ["hello"];
  }
  if ($x == 2) {
    return false;
  }
  return null;
}

function test_return_null() {
  $x = fork(return_null());
  $a = wait($x);
  var_dump($a);
}

function test_return_true() {
  $x = fork(return_true());
  $a = wait($x);
  var_dump($a);
}

function test_return_false() {
  $x = fork(return_false());
  $a = wait($x);
  var_dump($a);
}

function test_return_true_false_null() {
  for ($i = 0; $i < 3; ++$i) {
    $x = fork(return_true_false_null($i));
    $a = wait($x);
    var_dump($a);
  }
}

function test_return_false_null_array() {
  for ($i = 0; $i < 3; ++$i) {
    $x = fork(return_false_null_array($i));
    $a = wait($x);
    var_dump($a);
  }
}

test_return_null();
test_return_true();
test_return_false();
test_return_true_false_null();
test_return_false_null_array();
