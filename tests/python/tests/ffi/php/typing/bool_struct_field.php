<?php

function ensure_bool(bool $x) {}

function test() {
  $cdef = FFI::cdef('
    struct Foo {
      bool v;
    };
  ');
  $foo = $cdef->new('struct Foo');

  var_dump($foo->v);
  $foo->v = true;
  var_dump($foo->v);
  $foo->v = !$foo->v;

  ensure_bool($foo->v);
  var_dump(gettype($foo->v));

  if ($foo->v) {
    var_dump('yes');
  } else {
    var_dump('no');
  }
}

test();
