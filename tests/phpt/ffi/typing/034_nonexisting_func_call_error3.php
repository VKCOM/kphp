@kphp_should_fail k2_skip
/Method g\(\) not found in class ffi_scope<cdef\$u[a-f0-9]+_0>/
/Method g2\(\) not found in class ffi_scope<cdef\$u[a-f0-9]+_1>/
<?php

function test() {
  $cdef1 = FFI::cdef('void f();');
  $cdef1->g();
  $cdef2 = FFI::cdef('void f();');
  $cdef2->g2();
}

test();
