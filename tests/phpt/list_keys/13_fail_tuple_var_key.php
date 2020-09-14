@kphp_should_fail
/Type Error/
<?php
require_once 'kphp_tester_include.php';

// TODO: give more descriptive error message than just "type error"?

function test_var_key() {
  $tup = tuple(1, 'str', 5.6);
  $k = 0;
  list($k => $x) = $tup;
}

test_var_key();
