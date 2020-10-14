@kphp_should_fail
/TYPE INFERENCE ERROR/
<?php
require_once 'kphp_tester_include.php';

/**
 * @param $sh shape(x:int, ...)
 */
function printX($sh) {
  var_dump('x ' . $sh['x']);
}


printX(shape(['y' => 1]));
