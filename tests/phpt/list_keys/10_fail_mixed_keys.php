@kphp_should_fail
/Cannot mix keyed and unkeyed array entries in assignments/
<?php

function test_keyed_with_unkeyed(array $a) {
  list(0 => $x, $y) = $a;
}

test_keyed_with_unkeyed(['a', 'b']);
