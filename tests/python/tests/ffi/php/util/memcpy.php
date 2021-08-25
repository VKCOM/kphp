<?php

function test() {
  $cdef = FFI::cdef('
    struct Foo {
      int8_t b0;
      int8_t b1;
      int8_t b2;
      int8_t b3;
    };
    struct Bar {
      uint32_t x;
      struct Foo foo;
    };
  ');
  $foo = $cdef->new('struct Foo');
  $bar = $cdef->new('struct Bar');
  $bar2 = $cdef->new('struct Bar');
  $int8 = $cdef->new('int8_t');
  $uint16 = $cdef->new('uint16_t');

  $int8->cdata = 10;
  FFI::memcpy($uint16, $int8, 1);
  var_dump($uint16->cdata);

  $uint16->cdata = 9121;
  FFI::memcpy($int8, $uint16, 1);
  var_dump($int8->cdata);

  $bar->x = 49249;
  FFI::memcpy($foo, $bar, 4);
  var_dump([$foo->b0, $foo->b1, $foo->b2, $foo->b3]);

  FFI::memcpy($bar2, $bar, FFI::sizeof($bar));
  var_dump([$bar2->x, $bar2->foo->b0]);

  $bar->x = 237473249312;
  FFI::memcpy(FFI::addr($foo), $bar, 3);
  var_dump([$foo->b0, $foo->b1, $foo->b2, $foo->b3]);

  $bar->x = 0;
  FFI::memcpy($foo, FFI::addr($bar), 1);
  var_dump([$foo->b0, $foo->b1, $foo->b2, $foo->b3]);

  $bar->x = 0xffff;
  FFI::memcpy(FFI::addr($foo), FFI::addr($bar), 2);
  var_dump([$foo->b0, $foo->b1, $foo->b2, $foo->b3]);

  var_dump($bar->x);
  FFI::memcpy($bar, $foo, 4);
  var_dump($bar->x);

  $bar->x = 0;
  FFI::memcpy(FFI::addr($bar), $foo, 1);
  var_dump($bar->x);
}

test();
