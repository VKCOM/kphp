<?php

// 3 scopes (2 anonymous, 1 named) reference the same shared library
// and the same symbol; the shared library will be loaded only once

$cdef = FFI::cdef('
  int printf(const char *format, ...);
', 'libc.so.6');

$cdef->printf("hello1\n");

$cdef2 = FFI::cdef('
  int printf(const char *format, ...);
', 'libc.so.6');

$cdef2->printf("hello2\n");

$foo_scope = FFI::cdef('
  #define FFI_SCOPE "foo"
  int printf(const char *format, ...);
', 'libc.so.6');

$foo_scope->printf("hello3\n");

#ifndef KPHP
flush();
#endif
