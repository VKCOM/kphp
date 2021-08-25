<?php

$cdef = FFI::cdef('
  struct Foo { int8_t x; };
  struct Bar { struct Foo foo; };
');
$bar1 = $cdef->new('struct Bar');
$bar2 = $cdef->new('struct Bar');
$bar1->foo = $bar2->foo;
$bar2->foo->x = 310;
var_dump([$bar1->foo->x, $bar2->foo->x]);
$bar1->foo->x = 200;
var_dump([$bar1->foo->x, $bar2->foo->x]);
