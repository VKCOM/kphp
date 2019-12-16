@kphp_should_fail
/Expected type\:\sfalse\sActual type\:\sboolean/
<?php

/**
 * @kphp-infer
 * @param $x false
 */
function test($x) {
}

test(true);
