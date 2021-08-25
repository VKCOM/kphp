@kphp_should_fail
KPHP_ENABLE_FFI=1
/Too much arguments in function call/
<?php

$cdef = FFI::cdef('
  #define FFI_SCOPE "example"
  void f();
');

$cdef->f(1, 2);
