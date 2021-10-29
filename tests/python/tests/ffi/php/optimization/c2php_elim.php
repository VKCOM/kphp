<?php

function test() {
  $cdef = FFI::cdef('
    struct Foo { int64_t value; };
    struct Bar { struct Foo foo; int64_t value; };
    struct Baz { struct Bar bar; int64_t value; };
  ');
  $baz = $cdef->new('struct Baz');

  // The following lines should have 0 c2php conversion
  $baz->value = 1;
  $baz->bar->value = 2;
  $baz->bar->foo->value = 3;

  // The following lines should have 1 c2php conversion per line
  var_dump($baz->value);
  var_dump($baz->bar->value);
  var_dump($baz->bar->foo->value);
}

test();
