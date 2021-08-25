<?php

function test2() {
  $cdef = FFI::cdef('
    struct Foo {
      int8_t x;
      int32_t y;
      int64_t z;
    };

    union MyUnion {
      float f32;
      struct Foo foo;
      double f64;
    };
  ');

  $u = $cdef->new('union MyUnion');
  $u->f32 = 1.5;
  printf("%.5f\n", $u->f32);
  printf("%.5f\n", $u->f64);
  var_dump($u->foo->x);
  var_dump($u->foo->y);
  var_dump($u->foo->z);
  $u->foo->y = 9542394821843;
  printf("%.5f\n", $u->f32);
  printf("%.5f\n", $u->f64);
  var_dump($u->foo->x);
  var_dump($u->foo->y);
  var_dump($u->foo->z);
}

test2();
