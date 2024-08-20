@kphp_should_fail k2_skip
/Invalid php2c conversion: FFI\\CData_int8\*\* -> int8_t\*/
<?php

$cdef = FFI::cdef('
  void f(int8_t *x);
');

$x = FFI::new('int8_t');
$cdef->f(FFI::addr(FFI::addr($x)));
