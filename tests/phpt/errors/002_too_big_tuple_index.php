@kphp_should_fail
<?php

/**
 * @return tuple(int, int, int)
 */
function f() {
  return tuple(1, 2, 3);
}

$x = f()[3];
