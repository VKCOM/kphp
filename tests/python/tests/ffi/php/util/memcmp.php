<?php

require_once __DIR__ . '/../include/common.php';

function test() {
  $size = 10;
  $ints_a = FFI::new("int64_t[$size]");
  $ints_b = FFI::new("int64_t[$size]");
  for ($i = 0; $i < 5; $i++) {
    ffi_array_set($ints_a, $i, $i);
    ffi_array_set($ints_b, $i, $i);
  }
  ffi_array_set($ints_a, 5, 10);
  ffi_array_set($ints_b, 5, 20);

  $sizeof_int64 = 8;

  // PHP (and KPHP) allow using memcmp on FFI arrays
  var_dump(FFI::memcmp($ints_a, $ints_b, 0));
  var_dump(FFI::memcmp($ints_a, $ints_b, 1*$sizeof_int64));
  var_dump(FFI::memcmp($ints_a, $ints_b, 2*$sizeof_int64));
  var_dump(FFI::memcmp($ints_a, $ints_b, 6*$sizeof_int64));
  var_dump(FFI::memcmp($ints_b, $ints_a, 6*$sizeof_int64));

  $ints_a_ptr = FFI::cast('int64_t*', $ints_a);
  $ints_b_ptr = FFI::cast('int64_t*', $ints_b);

  // it's also allowed to compare pointer types
  var_dump(FFI::memcmp($ints_a_ptr, $ints_b_ptr, 0));
  var_dump(FFI::memcmp($ints_a_ptr, $ints_b_ptr, 1*$sizeof_int64));
  var_dump(FFI::memcmp($ints_a_ptr, $ints_b_ptr, 2*$sizeof_int64));
  var_dump(FFI::memcmp($ints_a_ptr, $ints_b_ptr, 6*$sizeof_int64));
  var_dump(FFI::memcmp($ints_b_ptr, $ints_a_ptr, 6*$sizeof_int64));

  $cdef = FFI::cdef('
    struct Foo {
      int64_t a;
      double b;
    };
  ');
  $foo1 = $cdef->new('struct Foo');
  $foo2 = $cdef->new('struct Foo');
  $sizeof_foo = FFI::sizeof($foo1);
  // comparing struct values is also OK as long as you know their sizes
  var_dump(FFI::memcmp($foo1, $foo2, $sizeof_foo));
  var_dump(FFI::memcmp($foo2, $foo1, $sizeof_foo));

  $foo1_ptr = FFI::cast('void*', FFI::addr($foo1));
  $foo2_ptr = FFI::cast('void*', FFI::addr($foo2));
  var_dump(FFI::memcmp($foo1_ptr, $foo2_ptr, $sizeof_foo));
  var_dump(FFI::memcmp($foo2_ptr, $foo1_ptr, $sizeof_foo));

  $foo1->a = 10;
  var_dump(FFI::memcmp($foo1, $foo2, $sizeof_foo));
  var_dump(FFI::memcmp($foo2, $foo1, $sizeof_foo));
  var_dump(FFI::memcmp($foo1_ptr, $foo2_ptr, $sizeof_foo));
  var_dump(FFI::memcmp($foo2_ptr, $foo1_ptr, $sizeof_foo));
  $foo2->a = 10;
  var_dump(FFI::memcmp($foo1, $foo2, $sizeof_foo));
  var_dump(FFI::memcmp($foo2, $foo1, $sizeof_foo));
  var_dump(FFI::memcmp($foo1_ptr, $foo2_ptr, $sizeof_foo));
  var_dump(FFI::memcmp($foo2_ptr, $foo1_ptr, $sizeof_foo));
  $foo1->b = 1.4;
  var_dump(FFI::memcmp($foo1, $foo2, $sizeof_foo));
  var_dump(FFI::memcmp($foo2, $foo1, $sizeof_foo));
  var_dump(FFI::memcmp($foo1_ptr, $foo2_ptr, $sizeof_foo));
  var_dump(FFI::memcmp($foo2_ptr, $foo1_ptr, $sizeof_foo));
  $foo2->b = 249.6;
  var_dump(FFI::memcmp($foo1, $foo2, $sizeof_foo));
  var_dump(FFI::memcmp($foo2, $foo1, $sizeof_foo));
  var_dump(FFI::memcmp($foo1_ptr, $foo2_ptr, $sizeof_foo));
  var_dump(FFI::memcmp($foo2_ptr, $foo1_ptr, $sizeof_foo));

  $double1 = FFI::new('double');
  $double2 = FFI::new('double');
  $sizeof_double = FFI::sizeof($double1);
  var_dump($sizeof_double);
  var_dump(FFI::memcmp($double1, $double2, $sizeof_double));
  var_dump(FFI::memcmp($double2, $double1, $sizeof_double));
  $double1->cdata = 14.3;
  var_dump(FFI::memcmp($double1, $double2, $sizeof_double));
  var_dump(FFI::memcmp($double2, $double1, $sizeof_double));
  $double2->cdata = 16.3;
  var_dump(FFI::memcmp($double1, $double2, $sizeof_double));
  var_dump(FFI::memcmp($double2, $double1, $sizeof_double));
}

function test2() {
  $cdef = FFI::cdef('
    struct A {
      int64_t ints[10];
    };
    struct B {
      struct A *a;
    };
  ');
  $a = $cdef->new('struct A');
  FFI::memset($a->ints, 7, FFI::sizeof($a->ints));
  for ($i = 0; $i < count($a->ints); $i++) {
    var_dump(ffi_array_get($a->ints, $i));
  }
  $b = $cdef->new('struct B');
  $b->a = FFI::addr($a);
  var_dump(FFI::memcmp($a->ints, $b->a->ints, FFI::sizeof($a->ints)));
  var_dump(FFI::memcmp($b->a->ints, $a->ints, FFI::sizeof($a->ints)));
  ffi_array_set($a->ints, 0, 13);
  var_dump(FFI::memcmp($a->ints, $b->a->ints, FFI::sizeof($a->ints)));
  var_dump(FFI::memcmp($b->a->ints, $a->ints, FFI::sizeof($a->ints)));
  $a_clone = clone $a;
  ffi_array_set($a_clone->ints, 0, 1242);
  var_dump(FFI::memcmp($a->ints, $b->a->ints, FFI::sizeof($a->ints)));
  var_dump(FFI::memcmp($b->a->ints, $a->ints, FFI::sizeof($a->ints)));
  var_dump(FFI::memcmp($a_clone->ints, $a->ints, FFI::sizeof($a->ints)));
  var_dump(FFI::memcmp($a->ints, $a_clone->ints, FFI::sizeof($a->ints)));
}

test();
test2();
