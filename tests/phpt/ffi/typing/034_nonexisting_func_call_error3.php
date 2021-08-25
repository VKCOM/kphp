@kphp_should_fail
KPHP_ENABLE_FFI=1
/Unknown function ->g\(\) of scope\$cdef\$ud1d16a4a0a7a19fb_0/
/Unknown function ->g\(\) of scope\$cdef\$ud1d16a4a0a7a19fb_1/
<?php

function test() {
  $cdef1 = FFI::cdef('void f();');
  $cdef1->g();
  $cdef2 = FFI::cdef('void f();');
  $cdef2->g();
}

test();
