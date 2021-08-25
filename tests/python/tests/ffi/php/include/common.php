<?php

#ifndef KPHP
define('kphp', 0);
if (false)
#endif
  define('kphp', 1);

function kphp_load_vector_lib() {
  if (kphp) {
    FFI::load(__DIR__ . '/../../c/vector.h');
  }
}

function kphp_load_pointers_lib() {
  if (kphp) {
    FFI::load(__DIR__ . '/../../c/pointers.h');
  }
}

/** @param ffi_cdata<vector, struct Vector2> $vec */
function print_vec($vec) {
  var_dump("<{$vec->x}, {$vec->y}>");
}

/** @param ffi_cdata<vector, struct Vector2Array> $arr */
function print_vec_array($arr) {
  $lib = FFI::scope('vector');
  for ($i = 0; $i < $arr->len; $i++) {
    print_vec($lib->vector2_array_get(FFI::addr($arr), $i));
  }
}
