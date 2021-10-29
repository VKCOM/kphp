<?php

function test() {
  $cdef1 = FFI::cdef('struct Foo {
    int32_t x;
  };');
  $cdef2 = FFI::cdef('struct Bar {
    int32_t y;
  };');

  $foo = $cdef1->new('struct Foo');
  $bar = $cdef2->new('struct Bar');
  $foo->x = 45;
  $bar->y = $foo->x;
  var_dump($foo->x, $bar->y);
}

test();
