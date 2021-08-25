@kphp_should_fail
KPHP_ENABLE_FFI=1
/Using result of void expression/
<?php

function test() {
  $lib = FFI::cdef('
      void f();
  ');

  $x = $lib->f();
}

test();
