@kphp_should_fail k2_skip
/return string from function/
/declared as @return int/
<?php

function test() {
  $cdef = FFI::cdef('
    void f(int (*g) ());
  ');
  $cdef->f(function () {
    return 'foo';
  });
}

test();
