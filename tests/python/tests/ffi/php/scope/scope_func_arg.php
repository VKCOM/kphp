<?php

require_once __DIR__ . '/../include/common.php';

kphp_load_vector_lib();

// Test that we can pass ffi scope (library object) around without
// losing the type information.

/**
 * @param ffi_scope<vector> $lib
 */
function use_lib($lib) {
  $vec = $lib->new('struct Vector2f');
  $vec->x = 5.5;
  $vec->y = -25.0;
  var_dump($vec->x, $vec->y);
}

function test() {
  $lib = FFI::scope('vector');
  use_lib($lib);
}

test();
