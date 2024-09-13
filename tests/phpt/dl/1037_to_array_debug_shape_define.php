@ok k2_skip
<?php
require_once 'kphp_tester_include.php';

class Foo {
  const FIELD = "foo";
}

function to_array_debug_shape_define() {
  /** @var shape(foo:string, bar:int) */
  $shape = shape([Foo::FIELD => "baz", "bar" => 42]);
  $dump = to_array_debug($shape);
  var_dump($dump);
}

to_array_debug_shape_define();
