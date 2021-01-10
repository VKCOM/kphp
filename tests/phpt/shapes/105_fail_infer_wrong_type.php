@kphp_should_fail
/pass shape\(x:int, y:int\) to argument \$sh of process/
<?php
require_once 'kphp_tester_include.php';

/**
 * @param shape(x:int, y?:string) $sh
 */
function process($sh) {
  var_dump($sh['x']);
  var_dump($sh['y']);
}

process(shape(['x' => 1, 'y' => 3]));
