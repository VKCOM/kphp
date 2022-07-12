<?php

require_once __DIR__ . '/../include/common.php';

function test() {
  $cdef = FFI::cdef('
    struct Foo {
      int8_t x;
      uint8_t y;
    };
  ');
  $foo = $cdef->new('struct Foo');

  FFI::memset(FFI::addr($foo), -22, 2);
  var_dump($foo->x);
  var_dump($foo->y);

  // PHP allows to call memset without FFI::addr(), so do we.
  FFI::memset($foo, 0xff, 2);
  var_dump($foo->x);
  var_dump($foo->y);

  // Write to only first field (the first byte).
  FFI::memset($foo, -10, 1);
  var_dump($foo->x);
  var_dump($foo->y);
}

function test_arrays() {
  $fixed_array = FFI::new('uint16_t[8]');
  FFI::memset($fixed_array, 0xff, 2 * 4);
  for ($i = 0; $i < count($fixed_array); $i++) {
    var_dump(ffi_array_get($fixed_array, $i));
  }
  FFI::memset($fixed_array, 15, 2 * 2);
  for ($i = 0; $i < count($fixed_array); $i++) {
    var_dump(ffi_array_get($fixed_array, $i));
  }

  $cdef = FFI::cdef('
    struct A {
      int64_t ints[10];
    };
    struct B {
      struct A *a;
    };
  ');
  $a = $cdef->new('struct A');
  FFI::memset($a->ints, 7, FFI::sizeof($a->ints));
  for ($i = 0; $i < count($a->ints); $i++) {
    var_dump(ffi_array_get($a->ints, $i));
  }
  $b = $cdef->new('struct B');
  $b->a = FFI::addr($a);
  FFI::memset($b->a->ints, 8, FFI::sizeof($b->a->ints));
  for ($i = 0; $i < count($b->a->ints); $i++) {
    var_dump(ffi_array_get($b->a->ints, $i));
    var_dump(ffi_array_get($a->ints, $i));
  }
}

test();
test_arrays();
