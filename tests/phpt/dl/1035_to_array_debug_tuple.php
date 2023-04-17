@ok
<?php
require_once 'kphp_tester_include.php';

class B {
  public $i = 88;
  public $b_tuple;
  function __construct() {
    $this->b_tuple = tuple(77, 'qax');
  }
}

class A {
  public $a_obj;

  function __construct() {
    $this->a_obj = new B;
  }
}

function to_array_debug_tuple(bool $with_class_names) {
  $tuple = tuple(42, new A);
  $dump = to_array_debug($tuple, $with_class_names);
  var_dump($dump);
}

to_array_debug_tuple(true);
to_array_debug_tuple(false);
