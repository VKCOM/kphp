@kphp_should_fail
KPHP_ENABLE_FFI=1
/Not enough arguments in function call/
<?php

$cdef = FFI::cdef('
  #define FFI_SCOPE "example"
  void f(int x, int y);
');

$cdef->f(1);
