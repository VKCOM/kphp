@kphp_should_fail
/Too many arguments to function call, expected 0, have 2/
<?php

$cdef = FFI::cdef('
  #define FFI_SCOPE "example"
  void f();
');

$cdef->f(1, 2);
