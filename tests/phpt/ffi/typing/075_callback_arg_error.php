@kphp_should_fail k2_skip
/Invalid callback passed/
<?php

function test() {
  $cdef = FFI::cdef('
    void f(void (*g) ());
  ');
  $cdef->f(10);
}

test();
