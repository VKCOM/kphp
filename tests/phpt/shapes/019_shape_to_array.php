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
#ifndef KPHP   // in KPHP shapes produce non-assoiative array at runtime
$dump['sh'] = [2, 'y', null];
#endif
var_dump($dump);

$a->setZ();
$dump = to_array_debug($a);
#ifndef KPHP   // in KPHP shapes produce non-assoiative array at runtime
$dump['sh'] = [2, 'y', [1,2,3]];
#endif
var_dump($dump);
