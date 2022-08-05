@kphp_should_fail
/Method g\(\) not found in class ffi_scope<cdef\$u[a-f0-9]+_0>/
<?php

$cdef = FFI::cdef('
  void f();
');

$cdef->g();
