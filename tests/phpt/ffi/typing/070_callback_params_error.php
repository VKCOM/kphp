@kphp_should_fail k2_skip
/Too few arguments in callback, expected 1, have 0/
<?php

function test() {
  $cdef = FFI::cdef('
    void f(void (*g) ());
  ');
  $cdef->f(function ($x) {});
}

test();
