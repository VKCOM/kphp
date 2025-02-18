@ok
<?php
require_once 'kphp_tester_include.php';

class B {
  public $i = 88;
  public $b_shape;
  function __construct() {
    $this->b_shape = shape(['baz' => 'qax']);
  }
}

class A {
  public $a_obj;

  function __construct() {
    $this->a_obj = new B;
  }
}

function to_array_debug_shape(bool $with_class_names) {
  $shape = shape(['foo' => 42, 'bar' => new A]);

  $dump = to_array_debug($shape, $with_class_names);
  #ifndef KPHP
  $dump = ['foo' => 42, 'bar' => ['a_obj' => ['i' => 88, 'b_shape' => ['baz' => 'qax']]]];
  if ($with_class_names) {
    $dump['bar']['__class_name'] = 'A';
    $dump['bar']['a_obj']['__class_name'] = 'B';
  }
  #endif
  var_dump($dump);
}

to_array_debug_shape(true);
to_array_debug_shape(false);
