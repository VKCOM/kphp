@kphp_should_fail
/Expected type\:\snull\sActual type\:\sint\|null/
<?php

/**
 * @kphp-infer
 * @param $x null
 */
function test($x) {
  ++$x;
}

test(null);
