@kphp_should_fail
/Only string const keys can be used, got 'int'/
<?php
require_once 'polyfills.php';

function test_int_key() {
  $tup = shape(['0' => 10]);
  list(0 => $x) = $tup;
}

test_int_key();
