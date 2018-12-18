@kphp_should_fail
<?php

function f() {
  return tuple(1, 2, 3);
}

$x = f()[3];
