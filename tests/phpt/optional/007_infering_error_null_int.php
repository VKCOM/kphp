@kphp_should_fail
/assign int to \$x, modifying a function argument/
<?php

/**
 * @param $x null
 */
function test($x) {
  ++$x;
}

test(null);
