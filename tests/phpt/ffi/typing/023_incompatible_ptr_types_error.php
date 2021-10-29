@kphp_should_fail
KPHP_ENABLE_FFI=1
/Invalid php2c conversion: FFI\\CData_int8\* -> int16_t\*/
<?php

$cdef = FFI::cdef('
  void f(int16_t *x);
');

$x = FFI::new('int8_t');
$cdef->f(FFI::addr($x));
