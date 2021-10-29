<?php

function test_scalar() {
  $int8 = FFI::new('int8_t');
  $int8->cdata = 10;
  $int8_copy = clone $int8;
  var_dump([$int8->cdata, $int8_copy->cdata]);
  $int8->cdata = 20;
  var_dump([$int8->cdata, $int8_copy->cdata]);
}

function test_struct() {
  $cdef = FFI::cdef('
    struct Foo {
      int32_t x;
      int16_t y;
    };
  ');
  $foo = $cdef->new('struct Foo');
  $foo->x = 42;
  $foo->y = 2;
  $foo_clone = clone $foo;
  var_dump([$foo->x, $foo->y, $foo_clone->x, $foo_clone->y]);
  $foo->x = 50;
  $foo->y = 9;
  var_dump([$foo->x, $foo->y, $foo_clone->x, $foo_clone->y]);
}

function test_struct_ref() {
  $cdef = FFI::cdef('
    struct Foo {
      int32_t x;
    };
    struct Bar {
      struct Foo foo;
    };
  ');
  $bar = $cdef->new('struct Bar');
  $foo = $bar->foo;
  $foo->x = 777;
  $foo_clone = clone $foo;
  var_dump([$foo->x, $foo_clone->x, $bar->foo->x]);
  $foo->x = 999;
  var_dump([$foo->x, $foo_clone->x, $bar->foo->x]);
  $foo_clone2 = clone $bar->foo;
  var_dump([$foo->x, $foo_clone->x, $bar->foo->x, $foo_clone2->x]);
  $foo_clone2->x = 1;
  $foo_clone->x = 2;
  var_dump([$foo->x, $foo_clone->x, $bar->foo->x, $foo_clone2->x]);
}

function test_pointer() {
  $cdef = FFI::cdef('
    struct Foo {
      int32_t x;
    };
    struct Bar {
      struct Foo *foo_ptr;
    };
  ');
  $foo = $cdef->new('struct Foo');
  $bar = $cdef->new('struct Bar');
  $bar->foo_ptr = FFI::addr($foo);
  $foo->x = 15;
  var_dump([$foo->x, $bar->foo_ptr->x]);
  $bar_clone = clone $bar;
  $foo->x = $foo->x * 2;
  var_dump([$foo->x, $bar->foo_ptr->x, $bar_clone->foo_ptr->x]);

  $ptr2 = FFI::addr($bar->foo_ptr);
  $ptr2_clone = clone $ptr2;
}

test_scalar();
test_struct();
test_struct_ref();
test_pointer();
