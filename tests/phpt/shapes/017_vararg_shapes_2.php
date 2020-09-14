@ok
<?php
require_once 'kphp_tester_include.php';

/**
 * @kphp-infer
 * @param $sh shape(x:int, ...)
 */
function printX($sh) {
  var_dump('x ' . $sh['x']);
}

/**
 * @kphp-infer
 * @param $sh shape(x:int, y:int, ...)
 */
function printXY($sh) {
  var_dump('y ' . $sh['y']);
  // it's ok to pass {x,y,...} to {x,...}
  printX($sh);
}

/**
 * @kphp-infer
 * @param $sh shape(x:int, y:int, z?:string)
 */
function printXYZ($sh) {
  if(isset($sh['z']))
    var_dump('z ' . $sh['z']);
  // it's ok to pass {x,y,z?} to {x,y,...}
  printXY($sh);
}

printXYZ(shape(['x' => 1, 'y' => 2, 'z' => 's']));
printXYZ(shape(['x' => 3, 'y' => 4]));
