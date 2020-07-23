@kphp_should_fail
<?php

/**
 * @kphp-infer
 * @return tuple(int, int, int)
 */
function f() {
  return tuple(1, 2, 3);
}

list($x, $y, $z, $t) = f();
