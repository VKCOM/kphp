@kphp_should_fail
KPHP_ENABLE_FFI=1
/Invalid php2c conversion: int -> const char\*/
<?php

$cdef = FFI::cdef('
  #define FFI_SCOPE "example"
  void f(const char *x);
');

$cdef->f(4);
