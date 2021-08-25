<?php

function test_vector_array() {
  $lib = FFI::scope('vector');

  $array = $lib->new('struct Vector2Array');

  $lib->vector2_array_alloc(FFI::addr($array), 10);

  $v1 = $lib->new('struct Vector2');
  $v1->x = 50.1;
  $v1->y = 60.2;
  $lib->vector2_array_set(FFI::addr($array), 0, $v1);

  $v1_copy = $lib->vector2_array_get(FFI::addr($array), 0);
  var_dump($v1_copy->x, $v1_copy->y);
  $v1_copy->x = 14.6;
  var_dump($v1_copy->x, $v1->x);

  $v1_copy2 = $lib->vector2_array_get(FFI::addr($array), 0);
  var_dump($v1_copy2->x);

  $lib->vector2_array_free(FFI::addr($array));
}

#ifndef KPHP
if (false)
#endif
  FFI::load(__DIR__ . '/../c/vector.h');

test_vector_array();
