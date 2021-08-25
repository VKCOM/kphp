@kphp_should_fail
KPHP_ENABLE_FFI=1
/Invalid php2c conversion: int -> char/
<?php

$cdef = FFI::cdef('
  void f(char);
');

$cdef->f(50);
