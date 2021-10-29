<?php

function test_vector2_float() {
  $lib = FFI::scope('vector');

  $vec = $lib->new('struct Vector2f');
  var_dump($vec->x, $vec->y);

  $vec->x = 5.5;
  $vec->y = -25.0;
  var_dump($vec->x, $vec->y);

  $lib->vector2f_abs(FFI::addr($vec));
  var_dump($vec->x, $vec->y);
}

#ifndef KPHP
if (false)
#endif
  FFI::load(__DIR__ . '/../c/vector.h');

test_vector2_float();
