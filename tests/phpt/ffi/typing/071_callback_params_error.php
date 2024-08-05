@kphp_should_fail k2_skip
/Too many arguments in callback, expected 0, have 1/
<?php

function test() {
  $cdef = FFI::cdef('
    void f(void (*g) (int x));
  ');
  $cdef->f(function () {});
}

test();
