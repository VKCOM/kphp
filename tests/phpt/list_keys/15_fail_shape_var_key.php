@kphp_should_fail
/Type Error/
<?php
require_once 'polyfills.php';

// TODO: give more descriptive error message than just "type error"?

function test_var_key() {
  $tup = shape(['k' => 10]);
  $k = 'k';
  list($k => $x) = $tup;
}

test_var_key();
