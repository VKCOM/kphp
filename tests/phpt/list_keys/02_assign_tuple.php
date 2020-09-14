@ok
<?php
require_once 'kphp_tester_include.php';

const ZERO = 0;

function test_assign_tuple1() {
  $tup = tuple(1, 'str', 5.6);
  list(0 => $x, 2 => $y) = $tup;
  var_dump($x, $y);
}

function test_assign_tuple2() {
  $tup = tuple(1, 'str', 5.6);
  list(2 => $x, 1 => $y) = $tup;
  var_dump($x, $y);
}

function test_assign_tuple3() {
  $tup = tuple(1, 'str', 5.6);
  list(ZERO => $x) = $tup;
  var_dump($x);
}

test_assign_tuple1();
test_assign_tuple2();
test_assign_tuple3();
