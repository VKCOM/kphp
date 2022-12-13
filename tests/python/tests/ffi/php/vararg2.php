<?php

require_once __DIR__ . '/include/common.php';

function test_printf2() {
  $cdef = FFI::cdef('
    struct Foo {
      int8_t i8;
      int32_t i32;
      int64_t i64;
      bool b;
    };
    struct Bar {
      struct Foo foo;
      struct Foo *foo_ptr;
    };
    int printf(const char *format, ...);
  ', 'libc.so.6');

  $foo = $cdef->new('struct Foo');
  $bar = $cdef->new('struct Bar');
  $bar->foo = $foo;
  $bar->foo_ptr = FFI::addr($bar->foo);

  $foo->i8 = 10;
  $cdef->printf("%d %d %d\n", $foo->i8, $bar->foo->i8, $bar->foo_ptr->i8);
  $bar->foo->i8 = 20;
  $bar->foo->i32 = 4215;
  $bar->foo->i64 = 2941529834;
  $bar->foo->b = true;

  $cdef->printf("%d %d %ld %d\n",
    $bar->foo->i8, $bar->foo->i32, $bar->foo->i64, (int)$bar->foo->b);
  $cdef->printf("%d %d %ld %d\n",
    $foo->i8, $foo->i32, $foo->i64, (int)$foo->b);
}

for ($i = 0; $i < 5; ++$i) {
  test_printf2();

  // PHP would require a flush() call to make C printf output stable.
#ifndef KPHP
  flush();
#endif
}
