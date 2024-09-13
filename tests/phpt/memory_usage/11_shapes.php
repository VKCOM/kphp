@ok k2_skip
<?php
require_once 'kphp_tester_include.php';

function test_simple_shape() {
  /** @var shape(x:int, y:string, z:int[]) */
  $sh;

#ifndef KPHP
  var_dump(0);
  return;
#endif
  $sh = shape(['x' => 5, 'y' => "hello", 'z' => [7, 42]]);
  var_dump(estimate_memory_usage($sh));
}

function test_shape_in_class() {
  class A {
    /** @var shape(x:int, y:string, z:int[]) */
    public $sh;

    function __construct() {
      $this->sh = shape(['y' => 'y', 'x' => 2, 'z' => [1, 2, 3]]);
    }
  }

#ifndef KPHP
  var_dump(32);
  return;
#endif
  $a = new A();
  var_dump(estimate_memory_usage($a));
}

test_simple_shape();
test_shape_in_class();
