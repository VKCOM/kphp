@kphp_should_fail
/TYPE INFERENCE ERROR/
<?php
require_once 'kphp_tester_include.php';

/**
 * @param shape(x:int, y?:string) $sh
 */
function process($sh) {
  var_dump($sh['x']);
  var_dump($sh['y']);
}

process(shape(['x' => 1, 'y' => 's']));
process(shape(['y' => 's']));
