@kphp_should_fail
/Variable \$shorthand_ternary_cond.* has Unknown type/
<?php

function test_shorthand_ternary_fails($lhs, $rhs) {
  var_dump($lhs ?: $rhs);
}

test_shorthand_ternary_fails(false, 42);
