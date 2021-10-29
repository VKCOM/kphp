<?php

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

test();
