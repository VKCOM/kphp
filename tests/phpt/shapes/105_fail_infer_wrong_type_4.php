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


printX(shape(['x' => 5, 'y' => 1]));
printX(shape(['x' => 5, 'z' => 1]));
printX(shape(['x' => 'str', 'zz' => 'zz']));
