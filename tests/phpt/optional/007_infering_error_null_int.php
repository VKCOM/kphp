@kphp_should_fail
/pass int to argument \$x of test/
<?php

/**
 * @param $x null
 */
function test($x) {
  ++$x;
}

test(null);
