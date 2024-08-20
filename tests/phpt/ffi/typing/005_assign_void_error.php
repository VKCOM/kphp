@kphp_should_fail k2_skip
/Using result of void expression/
<?php

function test() {
  $lib = FFI::cdef('
      void f();
  ');

  $x = $lib->f();
}

test();
