@kphp_should_fail
/Invalid callback passed/
<?php

function test() {
  $cdef = FFI::cdef('
    void f(void (*g) ());
  ');
  $cdef->f(10);
}

test();
