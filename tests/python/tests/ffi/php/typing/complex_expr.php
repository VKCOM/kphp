<?php

/**
 * @param ffi_scope<test> $lib
 * @return ffi_cdata<test, struct Foo>
 */
function new_foo($lib) {
  return $lib->new('struct Foo');
}

/** @return ffi_cdata<test, struct Foo> */
function foo_identity($foo) {
  return $foo;
}

/** @return ffi_cdata<test, struct Bar> */
function bar_identity($bar) {
  return $bar;
}

function test() {
  $cdef = FFI::cdef('
    #define FFI_SCOPE "test"
    struct Foo {
      int i;
    };
    struct Bar {
      struct Foo *foo_ptr;
    };
  ');
  $foo = new_foo($cdef);
  foo_identity($foo)->i = 10;
  var_dump(foo_identity($foo)->i);

  $bar = $cdef->new('struct Bar');
  $bar->foo_ptr = FFI::addr(foo_identity($foo));
  bar_identity($bar)->foo_ptr->i = 20;
  var_dump([foo_identity($foo)->i, bar_identity($bar)->foo_ptr->i]);
}

function testNestedAssign() {
    $cdef = FFI::cdef('struct Foo { int x; };');
    $foo2 = $foo = $cdef->new('struct Foo');
    $foo2->x = 10;
    var_dump($foo2->x);
}

function testExprNew() {
  $cdef = FFI::cdef('struct Foo { int i; };');
  var_dump($cdef->new('struct Foo')->i);
  $cdef->new('struct Foo')->i = 10;
}

function testExprAddr() {
  $cdef = FFI::cdef('struct Foo { int i; };');
  var_dump(FFI::addr($cdef->new('struct Foo'))->i);
  FFI::addr($cdef->new('struct Foo'))->i = 20;
}

test();
testNestedAssign();
testExprNew();
testExprAddr();
