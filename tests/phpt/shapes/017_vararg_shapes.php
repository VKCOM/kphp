@ok
<?php
require_once 'kphp_tester_include.php';

/**
 * @param $sh shape(x:int, ...)
 */
function printX($sh) {
  var_dump('x ' . $sh['x']);
}

// it's ok to pass {x,y} to {x,...}
printX(shape(['x' => 1, 'y' => 2]));
printX(shape(['x' => 5, 'z' => 'string']));
// $sh in printX() will be inferred as {x:int, y:int, z:string}, but due to '...' that's ok
