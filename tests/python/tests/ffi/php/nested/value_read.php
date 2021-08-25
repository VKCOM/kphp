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

    struct Qux {
      struct Baz baz;
    };
  ');
  $qux = $cdef->new('struct Qux');

  $qux->baz->bar->foo->integer = 50;
  $baz = $qux->baz;
  $bar = $baz->bar;
  $foo = $bar->foo;
  $foo2 = $baz->bar->foo;
  $foo3 = $qux->baz->bar->foo;

  var_dump($baz->bar->foo->integer);
  var_dump($bar->foo->integer);
  var_dump($foo->integer);
  var_dump($foo2->integer);
  var_dump($foo3->integer);
}

test();
