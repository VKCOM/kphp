<?php

function test() {
  $cdef = FFI::cdef('struct Foo { int8_t value; };');

  $foo = $cdef->new('struct Foo');
  $foo2 = $foo;

  $foo->value = 42;

  var_dump($foo->value);
  var_dump($foo2->value);

  $value = $foo->value;
  $value2 = $foo2->value;
  var_dump($value);
  var_dump($value2);
}

test();
