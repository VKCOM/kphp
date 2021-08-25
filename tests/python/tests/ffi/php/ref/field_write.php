<?php

$cdef = FFI::cdef('
  struct Foo {
    int64_t z;
  };
  struct Bar {
    struct Foo foo;
    int32_t y;
  };
  struct Baz {
    struct Bar bar;
    int16_t x;
  };
');

$baz = $cdef->new('struct Baz');
$bar = $baz->bar;
$foo = $bar->foo;

$baz->bar->foo->z = 10;
var_dump([$baz->bar->foo->z, $bar->foo->z, $foo->z]);

$bar->foo->z = 20;
var_dump([$baz->bar->foo->z, $bar->foo->z, $foo->z]);

$foo->z = 30;
var_dump([$baz->bar->foo->z, $bar->foo->z, $foo->z]);
