@kphp_should_fail k2_skip
/Invalid php2c conversion: int -> char/
<?php

$cdef = FFI::cdef('
  void f(char);
');

$cdef->f(50);
