<?php

function test() {
  $cdef = FFI::cdef('
    struct Foo {
      int64_t integer;
    };

    struct Bar {
      struct Foo foo;
    };

    struct Baz {
      struct Bar bar;
    };
  ');
  $baz = $cdef->new('struct Baz');
  $foo = $cdef->new('struct Foo');

  $foo->integer = 800;
  $baz->bar->foo = $foo;

  var_dump($baz->bar->foo->integer);
  var_dump($foo->integer);
}

test();
