@kphp_should_fail
KPHP_ENABLE_FFI=1
/Unknown function ->g\(\) of scope\$cdef\$u[a-f0-9]+_0/
/Unknown function ->g2\(\) of scope\$cdef\$u[a-f0-9]+_1/
<?php

function test() {
  $cdef1 = FFI::cdef('void f();');
  $cdef1->g();
  $cdef2 = FFI::cdef('void f();');
  $cdef2->g2();
}

test();
