@ok
<?php
require_once 'polyfills.php';

const ZERO = '0';

function test_assign_shape1() {
  $s = shape(['i' => 1, 's' => 'str']);
  list('s' => $x, 'i' => $y) = $s;
  var_dump($x, $y);
}

function test_assign_shape2() {
  $s = shape(['a' => 10, '0' => 5.6]);
  list('a' => $x, ZERO => $y) = $s;
  var_dump($x, $y);
}

test_assign_shape1();
test_assign_shape2();
