<?php

function ensure_string(string $x) {}

function test() {
  $cdef = FFI::cdef('
    struct Foo {
      char c;
    };
  ');
  $foo = $cdef->new('struct Foo');

  var_dump($foo->c);
  $foo->c = "x";
  var_dump($foo->c);

  ensure_string($foo->c);
  var_dump(gettype($foo->c));

  if ($foo->c === "x") {
    var_dump('yes');
  } else {
    var_dump('no');
  }
}

test();
