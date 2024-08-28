@ok k2_skip
<?php
require_once 'kphp_tester_include.php';

function to_array_debug_shape_nullable() {
  /** @var shape(foo:string, bar?:int) */
  $shape = shape(["foo" => "baz"]);
  $dump = to_array_debug($shape);

  #ifndef KPHP
  $dump = ['foo' => 'baz', 'bar' => null];
  #endif
  var_dump($dump);
}

to_array_debug_shape_nullable();
