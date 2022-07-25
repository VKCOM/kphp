@kphp_should_fail
/FFI callbacks should not capture any variables/
<?php

function test() {
  $cdef = FFI::cdef('
    void f(int (*g) ());
  ');

  $g = function () {
    return 10;
  };
  $cdef->f($g);
}

test();
