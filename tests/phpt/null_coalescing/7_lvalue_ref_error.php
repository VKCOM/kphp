@kphp_should_fail
/Can't make result of operation to be lvalue/
<?php

function foo(&$x) {
  $x = 1;
}

function test_ref_error() {
  $x = 1;
  $y = 2;

  foo($y ?? $x);
  var_dump($x);
  var_dump($y);
}

test_ref_error();
