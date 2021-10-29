<?php

require_once __DIR__ . '/../include/common.php';

kphp_load_pointers_lib();

function test_scalar() {
  $cdef = FFI::cdef('
    struct Bar {
      int64_t *x;
    };
  ');
  $lib = FFI::scope('pointers');
  $int = FFI::new('int64_t');
  $bar = $cdef->new('struct Bar');
  $bar->x = FFI::addr($int);
  $int->cdata = 10;
  var_dump([$lib->read_int64($bar->x), $int->cdata]);
  $int_clone = clone $int;
  $bar->x = FFI::addr($int_clone);
  $int->cdata = 20;
  var_dump([$lib->read_int64($bar->x), $int->cdata]);
  $int_ptr = FFI::addr($int);
  $bar->x = $int_ptr;
  $int->cdata = 30;
  var_dump([$lib->read_int64($bar->x), $int->cdata]);
  $bar->x = clone $int_ptr;
  $int->cdata = 40;
  var_dump([$lib->read_int64($bar->x), $int->cdata]);
}

function test_struct() {
  $cdef = FFI::cdef('
    struct Foo {
      int64_t x;
    };
    struct Bar {
      struct Foo *foo_ptr;
    };
    struct BazPtr {
      struct Bar *bar_ptr;
    };
    struct BazVal {
      struct Bar bar;
    };
  ');

  $foo = $cdef->new('struct Foo');
  $bar = $cdef->new('struct Bar');
  $bar->foo_ptr = FFI::addr($foo);
  $foo->x = 10;
  var_dump([$bar->foo_ptr->x, $foo->x]);
  $bar->foo_ptr->x = 20;
  var_dump([$bar->foo_ptr->x, $foo->x]);

  $bazptr = $cdef->new('struct BazPtr');
  $bazval = $cdef->new('struct BazVal');
  $bazptr->bar_ptr = FFI::addr($bar);
  $bazval->bar = $bar;
  $bar->foo_ptr->x = 30;
  var_dump([$bar->foo_ptr->x, $foo->x, $bazptr->bar_ptr->foo_ptr->x, $bazval->bar->foo_ptr->x]);
  $bazptr->bar_ptr->foo_ptr->x = 100;
  var_dump([$bar->foo_ptr->x, $foo->x, $bazptr->bar_ptr->foo_ptr->x, $bazval->bar->foo_ptr->x]);
  $bazval->bar->foo_ptr->x = 200;
  var_dump([$bar->foo_ptr->x, $foo->x, $bazptr->bar_ptr->foo_ptr->x, $bazval->bar->foo_ptr->x]);
}

test_scalar();
test_struct();
