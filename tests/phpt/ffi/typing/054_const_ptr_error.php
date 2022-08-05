@kphp_should_fail
/Invalid php2c conversion: const FFI\\CData_int32\* -> int32_t\*/
<?php

$cdef = FFI::cdef('
  void f_intptr(int*);
');

$const_intptr = FFI::new('const int*');
$cdef->f_intptr($const_intptr);
