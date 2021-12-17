<?php

require_once __DIR__ . '/../include/common.php';

kphp_load_vector_lib();

/** @return ffi_cdata<vector, struct Vector2f> */
function new_vector() {
  $lib = FFI::scope('vector');
  return $lib->new('struct Vector2f');
}

function test1() {
  $vec = new_vector();

  $fn = function() use ($vec) {
    $vec->x = 5.5;
    $vec->y = -25.0;
  };

  $fn();

  var_dump($vec->x, $vec->y);
}

function test2() {
  $cdef = \FFI::cdef('struct Vector2f { float x, y; };');

  $vec = $cdef->new('struct Vector2f');

  $fn = function() use ($vec) {
    $vec->x = 5.5;
    $vec->y = -25.0;
  };

  $fn();

  var_dump($vec->x, $vec->y);
}

function test3() {
  $lib = FFI::scope('vector');

  /** @var ffi_cdata<vector, struct Vector2f> */
  $vec = $lib->new('struct Vector2f');

  $fn = function() use ($vec) {
    $vec->x = 5.5;
    $vec->y = -25.0;
  };

  $fn();

  var_dump($vec->x, $vec->y);
}

function test4() {
  $lib = FFI::scope('vector');

  $vec = $lib->new('struct Vector2f');

  $fn = function() use ($vec) {
    $vec->x = 5.5;
    $vec->y = -25.0;
  };

  $fn();

  var_dump($vec->x, $vec->y);
}

test1();
test2();
test3();
test4();
