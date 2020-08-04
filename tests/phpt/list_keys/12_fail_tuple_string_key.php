@kphp_should_fail
/Only int const keys can be used, got 'string'/
<?php
require_once 'polyfills.php';

function test_string_key() {
  $tup = tuple(1, 'str', 5.6);
  list('a' => $x) = $tup;
}

test_string_key();
