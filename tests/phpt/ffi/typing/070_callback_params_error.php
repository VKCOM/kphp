@kphp_should_fail 
/Too many arguments for callback \(callable\(\): void\), expected 0, have 1/
<?php

function test() {
  $cdef = FFI::cdef('
    void f(void (*g) ());
  ');
  $cdef->f(function ($x) {});
}

test();
