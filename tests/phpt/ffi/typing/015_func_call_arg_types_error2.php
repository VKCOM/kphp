@kphp_should_fail k2_skip
/Invalid php2c conversion: int -> const char\*/
<?php

$cdef = FFI::cdef('
  #define FFI_SCOPE "example"
  void f(const char *x);
');

$cdef->f(4);
