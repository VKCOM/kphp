@kphp_should_fail
/Error combining shapes/
<?php
require_once 'polyfills.php';

class A {
  /** @var shape(x:int, y:A) */
  public $sh;
}

$a = new A;
$a->sh = shape(['x' => 1, 'y' => null]);
$a->sh = shape(['x' => 1, 'y' => 3]);
