<?php

#ifndef KPHP

if (getenv('KPHP_TESTS_POLYFILLS_REPO') === false) {
  die('$KPHP_TESTS_POLYFILLS_REPO is unset');
}
(function ($class_loader) {
  // add local classes that are placed near the running test
  [$script_path] = get_included_files();
  $class_loader->addPsr4("", dirname($script_path));
})(require_once getenv('KPHP_TESTS_POLYFILLS_REPO', true) . '/vendor/autoload.php');

define('kphp', 0);

if (false)
#endif
  define('kphp', 1);

/** @return ffi_scope<vector> */
function vector_lib() {
  /** @var ffi_scope<vector> $lib */
  static $lib = null;
  if (!$lib) {
    $lib = FFI::scope('vector');
  }
  return $lib;
}

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
