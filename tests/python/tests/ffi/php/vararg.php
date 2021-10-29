<?php

require_once __DIR__ . '/include/common.php';

function test_printf1() {
  $cdef = FFI::cdef('
    int printf(const char *format, ...);
  ', 'libc.so.6');

  $cdef->printf("hello\n");

  $cdef->printf("x=%ld\n", 10);
  $cdef->printf("x=%ld\n", PHP_INT_MAX);
  $cdef->printf("x=%ld y=%f\n", PHP_INT_MAX, 1.55);
  $cdef->printf(" %ld %f %ld %f \n", 1, 1.9, 1, 1.8);

  $cdef->printf("%s%s\n", "x", "y");

  $cdef->printf("%s: %0*d\n", implode('', ['ex', 'ample']), 5, 3);

  $int = FFI::new('int');
  $int->cdata = 5;
  $cdef->printf("%3ld\n", $int->cdata);

  $cdef->printf("true=%d false=%d int=%ld float=%.2f\n",
    true, false, 43, 43.0);
}

for ($i = 0; $i < 5; ++$i) {
  test_printf1();

  // PHP would require a flush() call to make C printf output stable.
#ifndef KPHP
  flush();
#endif
}
