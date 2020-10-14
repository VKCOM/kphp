@kphp_should_fail
/Expected type\:\snull\sActual type\:\sint\|null/
<?php

/**
 * @param $x null
 */
function test($x) {
  ++$x;
}

test(null);
