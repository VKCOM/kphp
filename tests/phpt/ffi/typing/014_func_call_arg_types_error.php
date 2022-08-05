@kphp_should_fail
/Invalid php2c conversion: string -> int64_t/
<?php

$cdef = FFI::cdef('
  #define FFI_SCOPE "example"
  void f(int64_t x);
');

$cdef->f("foo");
