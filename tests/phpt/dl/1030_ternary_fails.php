@kphp_should_fail
/Variable \$lhs has Unknown type/
<?php

function test_ternary_fails($lhs, $rhs) {
  var_dump($lhs ? $lhs : $rhs);
}

test_ternary_fails(false, 42);
