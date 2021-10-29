<?php

function test() {
  $lib = FFI::scope('vector');

  $vec = $lib->new('struct Vector2f');
  var_dump($vec->x, $vec->y);

  $vec2 = $vec;

  $vec2->x = 5.5;
  $vec2->y = -25.0;
  var_dump($vec2->x, $vec2->y);
  var_dump($vec->x, $vec->y);
}

#ifndef KPHP
if (false)
#endif
  FFI::load(__DIR__ . '/../c/vector.h');

test();
