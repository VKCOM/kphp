<?php

require_once __DIR__ . '/include/common.php';

kphp_load_vector_lib();

function test_vector2() {
  $lib = FFI::scope('vector');
  $arr = $lib->new('struct Vector2Array');

  $vec1 = $lib->new('struct Vector2');
  $vec1->x = 10.0;
  $vec1->y = 35.0;
  $vec2 = $lib->new('struct Vector2');
  $vec2->x = -4.5;
  $vec2->y = -1.9;

  $lib->vector2_array_init(FFI::addr($arr), 2, $vec1, $vec2);
  print_vec_array($arr);
  $lib->vector2_array_free(FFI::addr($arr));

  $lib->vector2_array_init_ptr(FFI::addr($arr), 2, FFI::addr($vec2), FFI::addr($vec1));
  print_vec_array($arr);
  $lib->vector2_array_free(FFI::addr($arr));
}

for ($i = 0; $i < 5; ++$i) {
  test_vector2();
}
