@ok
<?php
require_once 'kphp_tester_include.php';

class A {
  /** @var tuple(int, string) */
  var $t;
  /** @var shape(x:int, y:string, z?:int[]) */
  var $sh;

  function __construct() {
    $this->t = tuple(1, 's');
    $this->sh = shape(['y' => 'y', 'x' => 2]);
  }

  function setZ() {
    $this->sh = shape(['y' => 'y', 'x' => 2, 'z' => [1,2,3]]);
  }
}


$a = new A;
$dump = to_array_debug($a);
#ifndef KPHP // in KPHP order of keys will differs from php
$dump['sh'] = ['x' => 2, 'y' => 'y', 'z' => null];
#endif
var_dump($dump);

$a->setZ();
$dump = to_array_debug($a);
#ifndef KPHP // in KPHP order of keys will differs from php
$dump['sh'] = ['x' => 2, 'y' => 'y', 'z' => [1,2,3]];
#endif
var_dump($dump);
