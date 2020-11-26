@kphp_should_fail
/Function calls on the left-hand side of \?\?= are not supported/
<?php

function get_array(array $arr) : array {
  echo "get array\n";
  return $arr;
}

function test_fncall() {
  get_array([])[10] ??= 50;
  var_dump($a);
}

test_fncall();
