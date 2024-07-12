@kphp_should_fail k2_skip
/FFI callbacks should not capture any variables/
<?php

function test() {
  $cdef = FFI::cdef('
    void f(int (*g) ());
  ');

  $x = 10;
  $cdef->f(function () use ($x) {
    return $x;
  });
}

test();
