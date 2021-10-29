<?php

require_once __DIR__ . '/../include/common.php';

kphp_load_vector_lib();

// Test that we can pass ffi scope (library object) around without
// losing the type information.

class VectorLib {
  /** @var ffi_scope<vector> */
  private $lib;

  public function __construct() {
    $this->lib = FFI::scope('vector');
  }

  /** @return ffi_cdata<vector, struct Vector2f> */
  public function new_vector_float(float $x, float $y) {
    $vec = $this->lib->new('struct Vector2f');
    $vec->x = $x;
    $vec->y = $y;
    return $vec;
  }
}

function test() {
  $lib_wrapper = new VectorLib();
  $v = $lib_wrapper->new_vector_float(10, 20);
  var_dump($v->x, $v->y);
}

test();
