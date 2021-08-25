@kphp_should_fail
KPHP_ENABLE_FFI=1
/Unknown function ->g\(\) of scope\$example/
<?php

$cdef = FFI::cdef('
  #define FFI_SCOPE "example"
  void f();
');

$cdef->g();
