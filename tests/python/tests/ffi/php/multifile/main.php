<?php

require_once __DIR__ . '/../include/common.php';
require_once __DIR__ . '/FooUtils.php';
require_once __DIR__ . '/FooLib.php';
require_once __DIR__ . '/load_libvector.php';
require_once __DIR__ . '/load_libpointers.php';
require_once __DIR__ . '/VectorUtils.php';

/** @return ffi_cdata<foo, struct Foo> */
function test1() {
  $foolib = FooLib::instance();
  $foo = $foolib->new_foo();
  $foo->d = 23149.5321;
  $foo->f = (float)$foo->d;
  $foo->b = true;
  FooUtils::dump($foo);
  return $foo;
}

/**
 * @param ffi_cdata<foo, struct Foo> $foo
 * @return ffi_cdata<vector, struct Vector2Array>
 */
function test2($foo) {
  $libvector = FFI::scope('vector');

  $array = $libvector->new('struct Vector2Array');
  $libvector->vector2_array_alloc(FFI::addr($array), 2);

  $index = 1;
  $vec = $libvector->new('struct Vector2');
  $vec->x = $foo->d;
  $vec->y = $foo->d + 2.5;
  $libvector->vector2_array_set(FFI::addr($array), $index, $vec);

  return $array;
}

/**
 * @param ffi_cdata<vector, struct Vector2Array> $array
 */
function test2_2($array) {
  $libvector = FFI::scope('vector');

  $vec0 = $libvector->vector2_array_get(FFI::addr($array), 0);
  $vec1 = $libvector->vector2_array_get(FFI::addr($array), 1);

  VectorUtils::dump($vec0);
  VectorUtils::dump($vec1);

  $libvector->vector2_array_free(FFI::addr($array));
}

/** @param ffi_cdata<foo, struct Foo> $foo */
function test3($foo) {
  FooUtils::dump($foo);
  FooUtils::zero($foo);
  FooUtils::dump($foo);
}

/** @param ffi_cdata<foo, struct Foo> $foo */
function test4($foo) {
  $foo->d = 32.6;

  $i64 = FFI::new('int64_t');
  var_dump($i64->cdata);

  $libptr = FFI::scope('pointers');
  $libptr->write_int64(FFI::addr($i64), (int)$foo->d);
  var_dump($i64->cdata);
  var_dump($libptr->read_int64(FFI::addr($i64)));
}

function main() {
  $foo = test1();
  $array = test2($foo);
  test2_2($array);
  test3($foo);
  test4($foo);
}

main();

require_once __DIR__ . '/main2.php';
