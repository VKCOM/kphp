<?php

/**
 * @param ffi_cdata<vector, struct Vector2f> $vec
 */
function use_vector($vec) {
  $lib = FFI::scope('vector');

  var_dump($vec->x, $vec->y);

  $vec->x = 5.5;
  $vec->y = -25.0;
  var_dump($vec->x, $vec->y);

  $lib->vector2f_abs(FFI::addr($vec));
  var_dump($vec->x, $vec->y);
}

function test_vector2_float() {
  $lib = FFI::scope('vector');

  $v = $lib->new('struct Vector2f');
  use_vector($v);
}

#ifndef KPHP
if (false)
#endif
  FFI::load(__DIR__ . '/../c/vector.h');

test_vector2_float();
