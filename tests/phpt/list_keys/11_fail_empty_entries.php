@kphp_should_fail
/Cannot use empty array entries in keyed array assignment/
<?php

function test_empty_entries(array $a) {
  list(1 => $x, ,) = $a;
}

test_empty_entries(['a', 'b']);
