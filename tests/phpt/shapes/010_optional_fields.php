@ok
<?php
require_once 'polyfills.php';

class A {
  var $a = 10;
  public function p() { echo "A a = ", $this->a, "\n"; }
}

/**
 * $sh как shape будет автовыведен из всего, что туда передаётся
 */
/**
 * @kphp-infer
 * @param shape(a:A, x:int, y:mixed, z:int[]|null) $sh
 */
function printAll($sh) {
  echo "x ", $sh['x'], "\n";
  if($sh['y'] !== null)
    echo "y ", $sh['y'], "\n";
  if($sh['z'] !== null && count($sh['z']))
    echo "z ", $sh['z'][0], "\n";
  /** @var A */
  $a = $sh['a'];
  if($a)
    $a->p();
}

/**
 * @kphp-infer
 * @param $sh shape(x:int, y?:string, z?:int[], a?:A) Это защищает shape, чекает обязательные параметры + даёт assumption на ['a']
 */
function printAll_v2($sh) : void {
  echo "x ", $sh['x'], "\n";
  if($sh['y'] !== null)
    echo "y ", $sh['y'], "\n";
  if($sh['z'] !== null && count($sh['z']))
    echo "z ", $sh['z'][0], "\n";
  if($sh['a'])
    $sh['a']->p();
}

printAll(shape(['x' => 1]));
printAll(shape(['x' => 2, 'y' => 's2']));
printAll(shape(['x' => 2, 'y' => 123]));    // makes 'y' var
printAll(shape(['x' => 3, 'y' => 's3', 'z' => []]));
printAll(shape(['x' => 4, 'y' => 's4', 'z' => [1,2,3]]));
printAll(shape(['x' => 5, 'z' => [1,2,3]]));
printAll(shape(['x' => 6, 'z' => []]));
printAll(shape(['x' => 7, 'a' => new A]));
printAll(shape(['x' => 8, 'a' => new A, 'z' => [1,2]]));
printAll(shape(['x' => 9, 'a' => new A, 'y' => 's9', 'z' => [1,2]]));
printAll(shape(['x' => 9, 'a' => new A, 'y' => 's9', 'z' => [1,2]]));
printAll(shape(['y' => 's10', 'a' => new A, 'z' => [1,2,3], 'x' => 10]));
printAll(shape(['z' => [1,2,3], 'y' => 's11', 'a' => null, 'x' => 11]));

printAll_v2(shape(['x' => 1]));
//printAll_v2(shape(['y' => 's']));  // this is error: 'x' in arguments becomes |null, as not presented
printAll_v2(shape(['x' => 2, 'y' => 's2']));
printAll_v2(shape(['x' => 3, 'y' => 's3', 'z' => []]));
printAll_v2(shape(['x' => 4, 'y' => 's4', 'z' => [1,2,3]]));
printAll_v2(shape(['x' => 5, 'z' => [1,2,3]]));
printAll_v2(shape(['x' => 6, 'z' => []]));
printAll_v2(shape(['x' => 7, 'a' => new A]));
printAll_v2(shape(['x' => 8, 'a' => new A, 'z' => [1,2]]));
printAll_v2(shape(['x' => 9, 'a' => new A, 'y' => 's9', 'z' => [1,2]]));
printAll_v2(shape(['x' => 9, 'a' => new A, 'y' => 's9', 'z' => [1,2]]));
printAll_v2(shape(['y' => 's10', 'a' => new A, 'z' => [1,2,3], 'x' => 10]));
printAll_v2(shape(['z' => [1,2,3], 'y' => 's11', 'a' => null, 'x' => 11]));

/**
 * @kphp-infer
 * @param $sh shape(x:int, y?:int, z:string|null)
 */
function printXY($sh) {
  if(isset($sh['y'])) {
    var_dump('y ' . $sh['y']);
  }
  if(isset($sh['z'])) {
    var_dump('z ' . $sh['z']);
  }
  var_dump('x ' . $sh['x']);
}

// 'y' and 'z' never passed, restrictions say ok
printXY(shape(['x' => 1]));
printXY(shape(['x' => 2]));
