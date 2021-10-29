<?php

// When $lib argument is unspecified, KPHP implies
// that symbols will be available at link time,
// so the FFI scope lib type will be static.

$cdef = FFI::cdef('
  struct Foo {
    int64_t x;
  };

  struct Bar {
    int64_t y;
    struct Foo foo;
  };

  extern int errno;

  int printf(const char *format, ...);

  double sqrt(double x);
');

// Just to be sure that static libs handle multiple definitions as good as
// dynamic FFI scopes can.
$cdef2 = FFI::cdef('
  struct Foo {
    float x;
    float y;
  };
  struct Bar {
    struct Foo foo;
  };
  int printf(const char *format, ...);
');

$cdef->printf("hello %s/%3d\n", "ok", 1);
#ifndef KPHP
flush();
#endif
echo "OK\n";

$cdef->sqrt(-1.0);
var_dump($cdef->errno);

$foo = $cdef->new('struct Foo');
$foo->x = 10;
$bar = $cdef->new('struct Bar');
$bar->y = 20;
$bar->foo = $foo;
var_dump($foo->x);
var_dump($bar->y);
var_dump($bar->foo->x);

$foo2 = $cdef2->new('struct Foo');
$foo2->x = 4.0;
$foo2->y = -2.0;
$bar2 = $cdef2->new('struct Bar');
$bar2->foo = $foo2;
var_dump($foo2->x);
var_dump($foo2->y);
var_dump($bar2->foo->x);
var_dump($bar2->foo->y);
