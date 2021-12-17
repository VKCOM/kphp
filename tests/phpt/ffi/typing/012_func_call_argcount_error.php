@kphp_should_fail
KPHP_ENABLE_FFI=1
/Too many arguments to function call, expected 0, have 2/
<?php

$cdef = FFI::cdef('
  #define FFI_SCOPE "example"
  void f();
');

$cdef->f(1, 2);
