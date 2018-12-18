@kphp_should_fail
<?php

function f() {
  return tuple(1, 2, 3);
}

list($x, $y, $z, $t) = f();
