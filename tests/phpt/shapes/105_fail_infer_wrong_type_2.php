@kphp_should_fail
/assign shape\(x:int, y:int\) to A::\$sh/
/but it's declared as @var shape\(x:int, y:A\)/
<?php
require_once 'kphp_tester_include.php';

class A {
  /** @var shape(x:int, y:A) */
  public $sh;
}

$a = new A;
$a->sh = shape(['x' => 1, 'y' => null]);
$a->sh = shape(['x' => 1, 'y' => 3]);
