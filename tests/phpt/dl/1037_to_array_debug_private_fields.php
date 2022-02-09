@ok
<?php
require_once 'kphp_tester_include.php';

class B {
  public int $b_i = 11;
  protected int $b_j = 22;
  private int $b_k = 33;
}

class A {
  public int $a_i = 77;
  protected int $a_j = 88;
  private int $a_k = 99;

  public $shape;
  function __construct() {
    $this->shape = shape(['foo' => 42, 'bar' => new B]);
  }
}

function to_array_debug_private_fields() {
  $a = new A;
  var_dump(to_array_debug($a));
  var_dump(to_array_debug($a, false, true));
}

to_array_debug_private_fields();
