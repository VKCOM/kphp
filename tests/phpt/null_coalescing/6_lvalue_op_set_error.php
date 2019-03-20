@kphp_should_fail
/Can't make result of operation to be lvalue/
<?php

function test_op_set_error() {
  $x = 1;
  $y = null;

  $y ?? $x = 2;
  var_dump($x);
  var_dump($y);
}

test_op_set_error();
