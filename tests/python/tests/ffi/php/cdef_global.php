<?php

/** @param ffi_cdata<example, struct Foo> $foo */
function test_foo($foo) {
  var_dump($foo->i8);
}

/**
 * @param ffi_scope<example> $cdef
 * @return ffi_cdata<example, struct Foo> $foo
 */
function create_foo($cdef) {
  $foo = $cdef->new('struct Foo');
  return $foo;
}

$cdef = FFI::cdef('
  #define FFI_SCOPE "example"
  struct Foo {
    int8_t i8;
  };
');

$foo = $cdef->new('struct Foo');
var_dump($foo->i8);
$foo->i8 = 1;
test_foo($foo);

$foo2 = create_foo($cdef);
var_dump($foo2->i8);
$foo2->i8 = 1;
test_foo($foo2);

$int = FFI::new('int8_t');
$int->cdata = 530;
var_dump($int->cdata);
