<?php

require_once __DIR__ . '/include/common.php';

kphp_load_vector_lib();

function test_vector1() {
  $lib = FFI::scope('vector');
  $arr = $lib->new('struct Vector2Array');
  $lib->vector2_array_alloc(FFI::addr($arr), 3);
  print_vec_array($arr);
  $lib->vector2_array_fill(FFI::addr($arr),
    1.0, 2.0,
    5.0, 5.5,
    99.1, 99.2);
  print_vec_array($arr);
  $lib->vector2_array_fill(FFI::addr($arr),
      1.0, 2.0,
      0.0, 0.0,
      2.0, 1.0);
  print_vec_array($arr);
  $lib->vector2_array_free(FFI::addr($arr));
}

for ($i = 0; $i < 5; ++$i) {
  test_vector1();
}
