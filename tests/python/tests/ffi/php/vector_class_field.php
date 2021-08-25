<?php

class WithVector {
  /**
   * @var ffi_cdata<vector, struct Vector2f>
   */
  public $vec;
}

function use_vector(WithVector $wrapper) {
  $lib = FFI::scope('vector');

  $vec = $wrapper->vec;

  var_dump($vec->x, $vec->y);

  $vec->x = 5.5;
  $vec->y = -25.0;
  var_dump($vec->x, $vec->y);

  $lib->vector2f_abs(FFI::addr($vec));
  var_dump($vec->x, $vec->y);
}

function use_vector2(WithVector $wrapper) {
  $lib = FFI::scope('vector');

  var_dump($wrapper->vec->x, $wrapper->vec->y);

  $wrapper->vec->x = 5.5;
  $wrapper->vec->y = -25.0;
  var_dump($wrapper->vec->x, $wrapper->vec->y);

  $lib->vector2f_abs(FFI::addr($wrapper->vec));
  var_dump($wrapper->vec->x, $wrapper->vec->y);
}

function test_vector2_float() {
  $lib = FFI::scope('vector');

  $v = $lib->new('struct Vector2f');
  $wrapper = new WithVector();
  $wrapper->vec = $v;
  use_vector($wrapper);
  use_vector2($wrapper);
}

#ifndef KPHP
if (false)
#endif
  FFI::load(__DIR__ . '/../c/vector.h');

test_vector2_float();
